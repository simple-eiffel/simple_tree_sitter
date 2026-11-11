/*
 * The C tests of simple_tree_sitter.
 *
 *   sts_test <fixtures directory> <queries directory>
 *   sts_test --survey <language> <file> ...     how well a grammar parses real files
 *   sts_test --dump <language> <file>           the tree, spans and folds of one file
 */

#include "simple_tree_sitter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int s_checks = 0, s_failures = 0;

static void check(const char *a_name, int a_condition)
{
	s_checks++;
	if (!a_condition) {
		s_failures++;
		printf("  FAIL: %s\n", a_name);
	}
}

/* ------------------------------------------------------------------------- */
/* Text helpers                                                               */
/* ------------------------------------------------------------------------- */

typedef struct {
	uint32_t *chars;
	int32_t count;
} text32;

static text32 from_utf8(const char *a_text, size_t a_length)
{
	const unsigned char *s = (const unsigned char *)a_text;
	text32 l_result;
	size_t i = 0;
	l_result.chars = (uint32_t *)malloc((a_length + 1) * sizeof(uint32_t));
	l_result.count = 0;
	while (i < a_length) {
		uint32_t c = s[i++];
		int l_more = 0;
		if (c >= 0xF0) {
			c &= 0x07; l_more = 3;
		} else if (c >= 0xE0) {
			c &= 0x0F; l_more = 2;
		} else if (c >= 0xC0) {
			c &= 0x1F; l_more = 1;
		}
		while (l_more-- > 0 && i < a_length) {
			c = (c << 6) | (s[i++] & 0x3F);
		}
		l_result.chars[l_result.count++] = c;
	}
	return l_result;
}

static text32 from_string(const char *a_text)
{
	return from_utf8(a_text, strlen(a_text));
}

static char *read_file(const char *a_directory, const char *a_name, size_t *a_length)
{
	char l_path[2048];
	FILE *l_file;
	char *l_data;
	long l_size;
	snprintf(l_path, sizeof(l_path), "%s%s%s", a_directory ? a_directory : "", a_directory ? "/" : "", a_name);
	l_file = fopen(l_path, "rb");
	if (!l_file) {
		printf("  cannot read %s\n", l_path);
		*a_length = 0;
		return NULL;
	}
	fseek(l_file, 0, SEEK_END);
	l_size = ftell(l_file);
	fseek(l_file, 0, SEEK_SET);
	l_data = (char *)malloc((size_t)l_size + 1);
	*a_length = fread(l_data, 1, (size_t)l_size, l_file);
	l_data[*a_length] = 0;
	fclose(l_file);
	return l_data;
}

static text32 read_text(const char *a_directory, const char *a_name)
{
	size_t l_length = 0;
	char *l_data = read_file(a_directory, a_name, &l_length);
	text32 l_result = from_utf8(l_data ? l_data : "", l_length);
	free(l_data);
	return l_result;
}

static int32_t find(const text32 *a_text, const char *a_needle, int a_nth)
	/* Where the `a_nth' (from 1) occurrence of ASCII `a_needle' starts; -1 when none. */
{
	int32_t n = (int32_t)strlen(a_needle), i, j;
	for (i = 0; i + n <= a_text->count; i++) {
		for (j = 0; j < n && a_text->chars[i + j] == (uint32_t)(unsigned char)a_needle[j]; j++) {
		}
		if (j == n && --a_nth == 0) {
			return i;
		}
	}
	return -1;
}

static void line_and_column(const text32 *a_text, int32_t a_position, int32_t *a_line, int32_t *a_column)
{
	int32_t i;
	*a_line = 0;
	*a_column = 0;
	for (i = 0; i < a_position && i < a_text->count; i++) {
		if (a_text->chars[i] == '\n') {
			(*a_line)++;
			*a_column = 0;
		} else {
			(*a_column)++;
		}
	}
}

static int32_t line_count_of(const text32 *a_text)
{
	int32_t i, n = 1;
	for (i = 0; i < a_text->count; i++) {
		n += a_text->chars[i] == '\n';
	}
	return n;
}

/* ------------------------------------------------------------------------- */
/* Result helpers                                                             */
/* ------------------------------------------------------------------------- */

static int span_at(const int32_t *a_spans, int32_t a_count, int32_t a_line, int32_t a_column, int32_t *a_length)
	/* The style of the span that starts at `a_line', `a_column' (0 when none), and its length. */
{
	int32_t i;
	for (i = 0; i < a_count; i++) {
		if (a_spans[4 * i] == a_line && a_spans[4 * i + 1] == a_column) {
			if (a_length) {
				*a_length = a_spans[4 * i + 2];
			}
			return a_spans[4 * i + 3];
		}
	}
	if (a_length) {
		*a_length = 0;
	}
	return 0;
}

static int style_of(sts_doc *a_doc, const text32 *a_text, const char *a_needle, int a_nth, int32_t *a_length)
	/* The style of the span that starts where `a_needle' does. */
{
	int32_t l_position = find(a_text, a_needle, a_nth), l_line, l_column, n = 0;
	int32_t *l_spans;
	int l_style;
	if (l_position < 0) {
		return -1;
	}
	line_and_column(a_text, l_position, &l_line, &l_column);
	l_spans = sts_doc_spans(a_doc, l_line, l_line, &n);
	l_style = span_at(l_spans, n, l_line, l_column, a_length);
	sts_free(l_spans);
	return l_style;
}

static int spans_well_formed(sts_doc *a_doc, const int32_t *a_spans, int32_t a_count)
{
	int32_t i, l_line = -1, l_end = 0;
	for (i = 0; i < a_count; i++) {
		const int32_t *s = a_spans + 4 * i;
		int32_t l_start, l_next;
		if (s[0] < 0 || s[0] >= sts_doc_line_count(a_doc) || s[1] < 0 || s[2] < 1 || s[3] < 1 || s[3] > 10) {
			return 0;
		}
		if (s[0] < l_line || (s[0] == l_line && s[1] < l_end)) {
			return 0;
		}
		l_start = sts_doc_line_start(a_doc, s[0]);
		l_next = (s[0] + 1 < sts_doc_line_count(a_doc)) ? sts_doc_line_start(a_doc, s[0] + 1) - 1 : sts_doc_count(a_doc);
		if (l_start + s[1] + s[2] > l_next) {
			return 0;
		}
		l_line = s[0];
		l_end = s[1] + s[2];
	}
	return 1;
}

static int same_ints(const int32_t *a, int32_t a_count, const int32_t *b, int32_t b_count, int a_per_item)
{
	return a_count == b_count && (a_count == 0 || memcmp(a, b, (size_t)a_count * (size_t)a_per_item * sizeof(int32_t)) == 0);
}

static int same_as_fresh(sts_doc *a_doc, const char *a_language, const text32 *a_text, const char *a_what)
	/* Does `a_doc' have the tree, spans and folds a new document with `a_text' has? */
{
	sts_doc *l_fresh = sts_doc_new(a_language);
	char *l_mine, *l_theirs;
	int32_t *l_spans, *l_fresh_spans, *l_folds, *l_fresh_folds;
	int32_t n = 0, m = 0, l_last = line_count_of(a_text) - 1;
	int l_ok;
	sts_doc_set_text(l_fresh, a_text->chars, a_text->count);
	l_mine = sts_doc_sexp(a_doc);
	l_theirs = sts_doc_sexp(l_fresh);
	l_ok = sts_doc_count(a_doc) == a_text->count
		&& memcmp(sts_doc_text(a_doc), a_text->chars, (size_t)a_text->count * sizeof(uint32_t)) == 0
		&& sts_doc_line_count(a_doc) == l_last + 1;
	if (!l_ok) {
		printf("  %s: the text differs\n", a_what);
	}
	if (!l_mine || !l_theirs || strcmp(l_mine, l_theirs) != 0) {
		printf("  %s: the tree differs from a fresh parse\n", a_what);
		l_ok = 0;
	}
	l_spans = sts_doc_spans(a_doc, 0, l_last, &n);
	l_fresh_spans = sts_doc_spans(l_fresh, 0, l_last, &m);
	if (!same_ints(l_spans, n, l_fresh_spans, m, 4)) {
		printf("  %s: the spans differ from a fresh parse (%d and %d)\n", a_what, (int)n, (int)m);
		l_ok = 0;
	}
	l_folds = sts_doc_folds(a_doc, &n);
	l_fresh_folds = sts_doc_folds(l_fresh, &m);
	if (!same_ints(l_folds, n, l_fresh_folds, m, 2)) {
		printf("  %s: the folds differ from a fresh parse\n", a_what);
		l_ok = 0;
	}
	sts_free(l_mine);
	sts_free(l_theirs);
	sts_free(l_spans);
	sts_free(l_fresh_spans);
	sts_free(l_folds);
	sts_free(l_fresh_folds);
	sts_doc_free(l_fresh);
	return l_ok;
}

static void edit_both(sts_doc *a_doc, text32 *a_text, int32_t a_start, int32_t a_removed, const uint32_t *an_insert, int32_t an_inserted)
	/* Make the same change in `a_doc' and in the test's own copy `a_text'. */
{
	uint32_t *l_new = (uint32_t *)malloc(((size_t)a_text->count + (size_t)an_inserted + 1) * sizeof(uint32_t));
	memcpy(l_new, a_text->chars, (size_t)a_start * sizeof(uint32_t));
	if (an_inserted > 0) {
		memcpy(l_new + a_start, an_insert, (size_t)an_inserted * sizeof(uint32_t));
	}
	memcpy(l_new + a_start + an_inserted, a_text->chars + a_start + a_removed,
		(size_t)(a_text->count - a_start - a_removed) * sizeof(uint32_t));
	free(a_text->chars);
	a_text->chars = l_new;
	a_text->count += an_inserted - a_removed;
	check("edit accepted", sts_doc_edit(a_doc, a_start, a_removed, an_insert, an_inserted) == 1);
}

static void edit_ascii(sts_doc *a_doc, text32 *a_text, int32_t a_start, int32_t a_removed, const char *an_insert)
{
	text32 l_insert = from_string(an_insert);
	edit_both(a_doc, a_text, a_start, a_removed, l_insert.chars, l_insert.count);
	free(l_insert.chars);
}

static void forget_changes(sts_doc *a_doc)
	/* Take the lines `a_doc' owes and say they were used. */
{
	int32_t n = 0;
	sts_free(sts_doc_changed_lines(a_doc, &n));
	sts_doc_changes_taken(a_doc);
}

static int line_in_ranges(const int32_t *a_ranges, int32_t a_count, int32_t a_line)
{
	int32_t i;
	for (i = 0; i < a_count; i++) {
		if (a_ranges[2 * i] <= a_line && a_line <= a_ranges[2 * i + 1]) {
			return 1;
		}
	}
	return 0;
}

static uint32_t s_seed = 12345;

static int32_t random_below(int32_t a_limit)
{
	s_seed = s_seed * 1664525u + 1013904223u;
	return a_limit > 0 ? (int32_t)((s_seed >> 8) % (uint32_t)a_limit) : 0;
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                      */
/* ------------------------------------------------------------------------- */

static void test_languages(void)
{
	printf("languages\n");
	check("four languages", sts_language_count() == 4);
	check("first is eiffel", sts_language_name(0) && strcmp(sts_language_name(0), "eiffel") == 0);
	check("no fifth", sts_language_name(4) == NULL && sts_language_name(-1) == NULL);
	check("has json", sts_language_has("json") == 1);
	check("no cobol", sts_language_has("cobol") == 0 && sts_language_has(NULL) == 0);
	check("e is eiffel", sts_language_for_extension("e") && strcmp(sts_language_for_extension("e"), "eiffel") == 0);
	check("ECF is xml", sts_language_for_extension("ECF") && strcmp(sts_language_for_extension("ECF"), "xml") == 0);
	check("h is c", sts_language_for_extension("h") && strcmp(sts_language_for_extension("h"), "c") == 0);
	check("no txt", sts_language_for_extension("txt") == NULL && sts_language_for_extension("") == NULL);
	check("no partial extension", sts_language_for_extension("ec") == NULL && sts_language_for_extension("js") == NULL);
	check("unknown language gives no document", sts_doc_new("cobol") == NULL);
	check("keyword.control is a keyword", sts_style_for_capture("keyword.control") == STS_STYLE_KEYWORD);
	check("punctuation.bracket is an operator", sts_style_for_capture("punctuation.bracket") == STS_STYLE_OPERATOR);
	check("markup is plain", sts_style_for_capture("markup") == STS_STYLE_PLAIN && sts_style_for_capture("keywords") == STS_STYLE_PLAIN);
}

static void test_bundled_queries(const char *a_queries)
{
	static const char *const l_kinds[] = { "highlights.scm", "folds.scm", "brackets.scm" };
	int32_t i;
	int k;
	printf("bundled queries\n");
	for (i = 0; i < sts_language_count(); i++) {
		for (k = 0; k < 3; k++) {
			char l_name[256], l_message[200];
			size_t l_length = 0;
			char *l_source;
			int32_t l_offset = 0, l_kind = 0;
			sts_query *l_query;
			snprintf(l_name, sizeof(l_name), "%s/%s", sts_language_name(i), l_kinds[k]);
			l_source = read_file(a_queries, l_name, &l_length);
			l_query = sts_query_new(sts_language_name(i), l_source ? l_source : "", (int32_t)l_length, &l_offset, &l_kind, l_message, sizeof(l_message));
			if (!l_query) {
				printf("  %s: error %d at byte %d: %s\n", l_name, (int)l_kind, (int)l_offset, l_message);
			}
			check(l_name, l_query != NULL && sts_query_pattern_count(l_query) > 0);
			sts_query_free(l_query);
			free(l_source);
		}
	}
}

static void test_fixture(const char *a_fixtures, const char *a_language, const char *a_file)
	/* What every language must give for its fixture. */
{
	text32 l_text = read_text(a_fixtures, a_file);
	sts_doc *l_doc = sts_doc_new(a_language);
	int32_t *l_spans, *l_folds, *l_changed;
	int32_t n = 0, i, l_ok = 1;
	char *l_sexp;
	char l_name[100];

	printf("fixture %s\n", a_file);
	check("document made", l_doc != NULL);
	check("empty document has one line", sts_doc_count(l_doc) == 0 && sts_doc_line_count(l_doc) == 1);
	forget_changes(l_doc);
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	check("count", l_text.count > 0 && sts_doc_count(l_doc) == l_text.count);
	check("line count", sts_doc_line_count(l_doc) == line_count_of(&l_text));
	snprintf(l_name, sizeof(l_name), "%s parses without errors", a_file);
	check(l_name, sts_doc_has_error(l_doc) == 0);
	l_sexp = sts_doc_sexp(l_doc);
	check("has a tree", l_sexp != NULL && strlen(l_sexp) > 10);
	if (sts_doc_has_error(l_doc) && l_sexp) {
		printf("  %s\n", l_sexp);
	}
	sts_free(l_sexp);

	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("a new text changes every line", n == 1 && l_changed && l_changed[0] == 0 && l_changed[1] == sts_doc_line_count(l_doc) - 1);
	sts_free(l_changed);
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("asked again, the same lines are still owed", n == 1 && l_changed && l_changed[0] == 0 && l_changed[1] == sts_doc_line_count(l_doc) - 1);
	sts_free(l_changed);
	sts_doc_changes_taken(l_doc);
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("once taken, nothing is owed", n == 0 && l_changed == NULL);

	l_spans = sts_doc_spans(l_doc, 0, sts_doc_line_count(l_doc) - 1, &n);
	check("has spans", n > 5 && l_spans != NULL);
	check("spans are in order, apart and inside their lines", spans_well_formed(l_doc, l_spans, n));
	sts_free(l_spans);
	l_spans = sts_doc_spans(l_doc, -5, 100000, &i);
	check("line range is clipped", i == n);
	sts_free(l_spans);
	check("no spans for an empty range", sts_doc_spans(l_doc, 3, 2, &i) == NULL && i == 0);

	l_folds = sts_doc_folds(l_doc, &n);
	check("has folds", n > 0 && l_folds != NULL);
	for (i = 0; i < n; i++) {
		l_ok = l_ok && l_folds[2 * i] >= 0 && l_folds[2 * i] < l_folds[2 * i + 1] && l_folds[2 * i + 1] < sts_doc_line_count(l_doc)
			&& (i == 0 || l_folds[2 * i] > l_folds[2 * i - 2]
				|| (l_folds[2 * i] == l_folds[2 * i - 2] && l_folds[2 * i + 1] > l_folds[2 * i - 1]));
	}
	check("folds span lines, in order, no repeats", l_ok);
	sts_free(l_folds);

	check("same as a fresh parse", same_as_fresh(l_doc, a_language, &l_text, a_file));
	sts_doc_free(l_doc);
	free(l_text.chars);
}

static void test_random_edits(const char *a_fixtures, const char *a_language, const char *a_file)
	/* Random changes, each then undone: the document must come back to what a fresh parse gives. */
{
	text32 l_original = read_text(a_fixtures, a_file);
	text32 l_text = read_text(a_fixtures, a_file);
	sts_doc *l_doc = sts_doc_new(a_language);
	int l_round, l_ok = 1, l_shape = 1;
	char l_what[100];

	printf("random edits %s\n", a_file);
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	for (l_round = 0; l_round < 150 && l_ok; l_round++) {
		int32_t l_start = random_below(l_text.count + 1);
		int32_t l_removed = random_below(25);
		int32_t l_inserted = random_below(25);
		int32_t l_from = random_below(l_original.count - 25);
		uint32_t l_saved[32];
		int32_t *l_spans, n = 0;
		if (l_removed > l_text.count - l_start) {
			l_removed = l_text.count - l_start;
		}
		memcpy(l_saved, l_text.chars + l_start, (size_t)l_removed * sizeof(uint32_t));
		edit_both(l_doc, &l_text, l_start, l_removed, l_original.chars + l_from, l_inserted);
		l_spans = sts_doc_spans(l_doc, 0, sts_doc_line_count(l_doc) - 1, &n);
		l_shape = l_shape && sts_doc_count(l_doc) == l_text.count && sts_doc_line_count(l_doc) == line_count_of(&l_text)
			&& spans_well_formed(l_doc, l_spans, n);
		sts_free(l_spans);
		edit_both(l_doc, &l_text, l_start, l_inserted, l_saved, l_removed);
		snprintf(l_what, sizeof(l_what), "%s round %d (start %d, removed %d, inserted %d)", a_file, l_round, (int)l_start, (int)l_removed, (int)l_inserted);
		l_ok = same_as_fresh(l_doc, a_language, &l_original, l_what);
	}
	check("the text and spans stay well formed while broken", l_shape);
	check("every undone edit gives the fresh parse", l_ok);
	sts_doc_free(l_doc);
	free(l_text.chars);
	free(l_original.chars);
}

static void test_eiffel(const char *a_fixtures)
{
	text32 l_text = read_text(a_fixtures, "sample.e");
	sts_doc *l_doc = sts_doc_new("eiffel");
	int32_t l_length = 0, n = 0, l_line, l_column, l_position;
	int32_t l_pair[4], l_info[7];
	int32_t *l_spans, *l_folds, *l_changed;
	const char *l_type;
	int32_t i, l_found;

	printf("eiffel\n");
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	forget_changes(l_doc);

	/* Positions count characters: line 0 has an accent, a symbol and an emoji. */
	l_spans = sts_doc_spans(l_doc, 0, 0, &n);
	check("first line is one comment span", n == 1 && l_spans[0] == 0 && l_spans[1] == 0 && l_spans[3] == STS_STYLE_COMMENT);
	check("the comment's length counts characters", n == 1 && l_spans[2] == find(&l_text, "\n", 1));
	sts_free(l_spans);
	check("class is a keyword", style_of(l_doc, &l_text, "class\n", 1, &l_length) == STS_STYLE_KEYWORD && l_length == 5);
	check("SAMPLE is a type", style_of(l_doc, &l_text, "SAMPLE", 1, &l_length) == STS_STYLE_TYPE && l_length == 6);
	check("STRING is a type", style_of(l_doc, &l_text, "STRING;", 1, &l_length) == STS_STYLE_TYPE && l_length == 6);
	check("42 is a number", style_of(l_doc, &l_text, "42", 1, &l_length) == STS_STYLE_NUMBER && l_length == 2);
	check("2.5 is a number", style_of(l_doc, &l_text, "2.5", 1, &l_length) == STS_STYLE_NUMBER && l_length == 3);
	check("'x' is a string style", style_of(l_doc, &l_text, "'x'", 1, &l_length) == STS_STYLE_STRING && l_length == 3);
	check("a string with wide characters", style_of(l_doc, &l_text, "\"na", 1, &l_length) == STS_STYLE_STRING && l_length == 17);
	check("the keyword after it", style_of(l_doc, &l_text, "ensure", 1, &l_length) == STS_STYLE_KEYWORD && l_length == 6);
	check(":= is an operator", style_of(l_doc, &l_text, ":= a_name", 1, &l_length) == STS_STYLE_OPERATOR && l_length == 2);
	check("a feature's name is a function", style_of(l_doc, &l_text, "total (", 1, &l_length) == STS_STYLE_FUNCTION && l_length == 5);
	check("an argument is a variable", style_of(l_doc, &l_text, "a_name:", 1, &l_length) == STS_STYLE_VARIABLE && l_length == 6);
	check("Result is a keyword", style_of(l_doc, &l_text, "Result :=", 1, &l_length) == STS_STYLE_KEYWORD && l_length == 6);
	check("True and False would be numbers", sts_style_for_capture("boolean") == STS_STYLE_NUMBER);
	check("a header comment", style_of(l_doc, &l_text, "-- Sum of", 1, &l_length) == STS_STYLE_COMMENT && l_length == 21);
	check("Void is a keyword", style_of(l_doc, &l_text, "Void", 1, &l_length) == STS_STYLE_KEYWORD);
	check("and then is one keyword", style_of(l_doc, &l_text, "and then", 1, &l_length) == STS_STYLE_KEYWORD && l_length == 8);

	/* Folds: the class, and the feature `total'. */
	l_folds = sts_doc_folds(l_doc, &n);
	line_and_column(&l_text, find(&l_text, "total (", 1), &l_line, &l_column);
	l_found = 0;
	for (i = 0; i < n; i++) {
		l_found = l_found || (l_folds[2 * i] == l_line && l_folds[2 * i + 1] > l_line + 15);
	}
	check("the feature total folds", l_found);
	line_and_column(&l_text, find(&l_text, "if a_values", 1), &l_line, &l_column);
	l_found = 0;
	for (i = 0; i < n; i++) {
		l_found = l_found || (l_folds[2 * i] == l_line && l_folds[2 * i + 1] == l_line + 6);
	}
	check("the conditional folds to its end", l_found);
	sts_free(l_folds);

	/* Brackets. */
	l_position = find(&l_text, "(v.out)", 1);
	check("an open parenthesis finds its close", sts_doc_bracket_at(l_doc, l_position, l_pair) == 1
		&& l_pair[0] == l_position && l_pair[1] == 1 && l_pair[2] == l_position + 6 && l_pair[3] == 1);
	check("just after the close finds the same pair", sts_doc_bracket_at(l_doc, l_position + 7, l_pair) == 1
		&& l_pair[0] == l_position && l_pair[2] == l_position + 6);
	check("inside finds nothing", sts_doc_bracket_at(l_doc, l_position + 3, l_pair) == 0);
	l_position = find(&l_text, "if a_values", 1);
	check("if finds its end", sts_doc_bracket_at(l_doc, l_position, l_pair) == 1 && l_pair[0] == l_position && l_pair[1] == 2
		&& l_pair[2] == find(&l_text, "end\n\t\t\t\ti := i + 1", 1) && l_pair[3] == 3);
	l_position = find(&l_text, "class\n", 1);
	check("class finds the last end", sts_doc_bracket_at(l_doc, l_position, l_pair) == 1 && l_pair[2] == l_text.count - 4 && l_pair[3] == 3);
	check("the last end finds class", sts_doc_bracket_at(l_doc, l_text.count - 2, l_pair) == 1 && l_pair[0] == l_position && l_pair[1] == 5);
	l_position = find(&l_text, "do\n\t\t\tname :=", 1);
	check("do finds its end", sts_doc_bracket_at(l_doc, l_position, l_pair) == 1 && l_pair[0] == l_position
		&& l_pair[2] == find(&l_text, "end\n\nfeature -- Access", 1));
	check("out of range finds nothing", sts_doc_bracket_at(l_doc, -1, l_pair) == 0 && sts_doc_bracket_at(l_doc, l_text.count + 5, l_pair) == 0);

	/* Nodes. */
	l_position = find(&l_text, "SAMPLE", 1);
	l_type = sts_doc_node_at(l_doc, l_position + 2, l_info);
	check("the node at SAMPLE is a class name", l_type && strcmp(l_type, "class_name") == 0 && l_info[0] == l_position && l_info[1] == 6
		&& l_info[2] == 5 && l_info[3] == 5 && l_info[4] == 1 && l_info[5] == 0 && l_info[6] == 0);
	check("no node outside the text", sts_doc_node_at(l_doc, l_text.count, l_info) == NULL);

	/* An edit inside one line changes that line, and the result is what a fresh parse gives. */
	l_position = find(&l_text, "count := a_count + 42", 1);
	line_and_column(&l_text, l_position, &l_line, &l_column);
	edit_ascii(l_doc, &l_text, l_position + 19, 2, "1234");
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("a small edit changes its line", n >= 1 && line_in_ranges(l_changed, n, l_line));
	check("and not the whole text", n >= 1 && !(l_changed[0] == 0 && l_changed[2 * n - 1] == sts_doc_line_count(l_doc) - 1));
	sts_free(l_changed);
	sts_doc_changes_taken(l_doc);
	check("number edit: same as fresh", same_as_fresh(l_doc, "eiffel", &l_text, "number edit"));
	check("1234 is a number", style_of(l_doc, &l_text, "1234", 1, &l_length) == STS_STYLE_NUMBER && l_length == 4);

	/* Opening a string turns what follows into a string; closing it turns it back. */
	l_position = find(&l_text, "name := a_name", 1);
	line_and_column(&l_text, l_position, &l_line, &l_column);
	edit_ascii(l_doc, &l_text, l_position + 8, 0, "\"");
	check("open string: same as fresh", same_as_fresh(l_doc, "eiffel", &l_text, "open string"));
	forget_changes(l_doc);
	edit_ascii(l_doc, &l_text, l_position + 8, 1, "");
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("closing it changes that line", n >= 1 && line_in_ranges(l_changed, n, l_line));
	sts_free(l_changed);
	sts_doc_changes_taken(l_doc);
	check("closed string: same as fresh", same_as_fresh(l_doc, "eiffel", &l_text, "closed string"));
	check("no errors again", sts_doc_has_error(l_doc) == 0);

	/* New lines: a whole feature added, lines counted again. */
	l_position = find(&l_text, "invariant", 1);
	n = sts_doc_line_count(l_doc);
	edit_ascii(l_doc, &l_text, l_position, 0, "feature -- More\n\n\tmore: INTEGER\n\t\tdo\n\t\t\tResult := 7\n\t\tend\n\n");
	check("seven more lines", sts_doc_line_count(l_doc) == n + 7);
	check("new feature: same as fresh", same_as_fresh(l_doc, "eiffel", &l_text, "new feature"));
	check("more is a function", style_of(l_doc, &l_text, "more:", 1, &l_length) == STS_STYLE_FUNCTION);
	check("still no errors", sts_doc_has_error(l_doc) == 0);

	/* Several edits before the changed lines are taken: all of them are reported. */
	forget_changes(l_doc);
	edit_ascii(l_doc, &l_text, find(&l_text, "SAMPLE", 1), 6, "EXAMPLE");
	l_position = find(&l_text, "Result := 7", 1);
	edit_ascii(l_doc, &l_text, l_position + 10, 1, "8");
	l_changed = sts_doc_changed_lines(l_doc, &n);
	line_and_column(&l_text, l_position, &l_line, &l_column);
	check("both edits' lines are reported", n >= 2 && line_in_ranges(l_changed, n, 5) && line_in_ranges(l_changed, n, l_line));
	sts_free(l_changed);
	sts_doc_changes_taken(l_doc);

	/* Edits at the very ends, and refused ones. */
	edit_ascii(l_doc, &l_text, 0, 0, "-- first\n");
	edit_ascii(l_doc, &l_text, l_text.count, 0, "-- last");
	check("ends: same as fresh", same_as_fresh(l_doc, "eiffel", &l_text, "ends"));
	check("an edit past the end is refused", sts_doc_edit(l_doc, l_text.count + 1, 0, NULL, 0) == 0);
	check("a removal past the end is refused", sts_doc_edit(l_doc, l_text.count - 1, 2, NULL, 0) == 0);
	check("a negative edit is refused", sts_doc_edit(l_doc, -1, 0, NULL, 0) == 0);
	check("refused edits change nothing", same_as_fresh(l_doc, "eiffel", &l_text, "refused"));
	edit_ascii(l_doc, &l_text, 0, l_text.count, "");
	check("everything removed", sts_doc_count(l_doc) == 0 && sts_doc_line_count(l_doc) == 1);
	check("empty: no spans", sts_doc_spans(l_doc, 0, 0, &n) == NULL && n == 0);

	sts_doc_free(l_doc);
	free(l_text.chars);
}

static void test_eiffel_constructs(const char *a_fixtures)
	/* The constructs the Eiffel grammar is patched for (grammars/eiffel/simple_eiffel.patch). */
{
	text32 l_text = read_text(a_fixtures, "constructs.e");
	sts_doc *l_doc = sts_doc_new("eiffel");
	int32_t l_length = 0, n = 0, l_position, l_line, l_column;
	int32_t l_info[7];
	int32_t *l_folds, *l_spans;
	const char *l_type;
	char *l_sexp;

	printf("eiffel constructs\n");
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	check("constructs.e has no error", sts_doc_has_error(l_doc) == 0);

	check("a base 16 integer", style_of(l_doc, &l_text, "0xFF_FF", 1, &l_length) == STS_STYLE_NUMBER && l_length == 7);
	check("a base 8 integer", style_of(l_doc, &l_text, "0c777", 1, &l_length) == STS_STYLE_NUMBER && l_length == 5);
	check("a base 2 integer", style_of(l_doc, &l_text, "0b1010_0101", 1, &l_length) == STS_STYLE_NUMBER && l_length == 11);
	check("a decimal integer", style_of(l_doc, &l_text, "1_000", 1, &l_length) == STS_STYLE_NUMBER && l_length == 5);
	check("a base 16 integer in an expression", style_of(l_doc, &l_text, "0x10", 1, &l_length) == STS_STYLE_NUMBER && l_length == 4);

	check("a special character", style_of(l_doc, &l_text, "'%N'", 1, &l_length) == STS_STYLE_STRING && l_length == 4);
	check("a character code", style_of(l_doc, &l_text, "'%/31/'", 1, &l_length) == STS_STYLE_STRING && l_length == 7);
	check("a character code in base 16", style_of(l_doc, &l_text, "'%/0x1F600/'", 1, &l_length) == STS_STYLE_STRING && l_length == 12);
	check("a space", style_of(l_doc, &l_text, "' '", 1, &l_length) == STS_STYLE_STRING && l_length == 3);
	check("an at sign", style_of(l_doc, &l_text, "'@'", 1, &l_length) == STS_STYLE_STRING && l_length == 3);
	check("a quote", style_of(l_doc, &l_text, "'%''", 1, &l_length) == STS_STYLE_STRING && l_length == 4);
	check("a double quote", style_of(l_doc, &l_text, "'\"'", 1, &l_length) == STS_STYLE_STRING && l_length == 3);
	check("a percent sign", style_of(l_doc, &l_text, "'%%'", 1, &l_length) == STS_STYLE_STRING && l_length == 4);
	check("a minus sign", style_of(l_doc, &l_text, "'-'", 1, &l_length) == STS_STYLE_STRING && l_length == 3);
	line_and_column(&l_text, find(&l_text, "accented: CHARACTER_32 = ", 1) + 25, &l_line, &l_column);
	l_spans = sts_doc_spans(l_doc, l_line, l_line, &n);
	check("an accented letter", span_at(l_spans, n, l_line, l_column, &l_length) == STS_STYLE_STRING && l_length == 3);
	sts_free(l_spans);
	l_position = find(&l_text, "'%/31/'", 1);
	l_type = sts_doc_node_at(l_doc, l_position + 1, l_info);
	check("the code is one character node", l_type && strcmp(l_type, "character") == 0 && l_info[0] == l_position + 1 && l_info[1] == 5);

	check("detachable before TUPLE is a keyword", style_of(l_doc, &l_text, "detachable TUPLE [row", 1, &l_length) == STS_STYLE_KEYWORD && l_length == 10);
	check("TUPLE is a type", style_of(l_doc, &l_text, "TUPLE [row", 1, &l_length) == STS_STYLE_TYPE && l_length == 5);
	check("a label's type is a type", style_of(l_doc, &l_text, "INTEGER]", 1, &l_length) == STS_STYLE_TYPE && l_length == 7);
	l_sexp = sts_doc_sexp(l_doc);
	check("the empty tuple is a manifest tuple with no items", l_sexp && strstr(l_sexp, "(manifest_tuple)") != NULL);
	sts_free(l_sexp);

	check("a string after its type", style_of(l_doc, &l_text, "\"#\"", 1, &l_length) == STS_STYLE_STRING && l_length == 3);
	check("the type before it", style_of(l_doc, &l_text, "STRING_32} \"#\"", 1, &l_length) == STS_STYLE_TYPE && l_length == 9);
	check("an integer after its type", style_of(l_doc, &l_text, "0x1F)", 1, &l_length) == STS_STYLE_NUMBER && l_length == 4);

	check("separate is a keyword", style_of(l_doc, &l_text, "separate CONSTRUCTS\n", 1, &l_length) == STS_STYLE_KEYWORD && l_length == 8);
	check("the type after it", style_of(l_doc, &l_text, "CONSTRUCTS\n\n\tworkers", 1, &l_length) == STS_STYLE_TYPE && l_length == 10);
	check("separate in a generic", style_of(l_doc, &l_text, "separate CONSTRUCTS]", 1, &l_length) == STS_STYLE_KEYWORD);
	check("separate in an argument", style_of(l_doc, &l_text, "separate READABLE", 1, &l_length) == STS_STYLE_KEYWORD);

	/* The class folds from `class' to its `end': no error breaks the structure. */
	l_folds = sts_doc_folds(l_doc, &n);
	check("the class folds to the last line", n >= 1 && l_folds[0] == 1 && l_folds[1] == sts_doc_line_count(l_doc) - 2);
	sts_free(l_folds);

	sts_doc_free(l_doc);
	free(l_text.chars);
}

static void test_results_in_blocks(const char *a_fixtures)
	/* The two results a worker hands on as blocks: spans for several ranges, and a bracket pair. */
{
	text32 l_text = read_text(a_fixtures, "sample.e");
	sts_doc *l_doc = sts_doc_new("eiffel");
	static const int32_t l_ranges[] = { 0, 4, 18, 28, 43, 43, 73, 400 };
	int32_t *l_all, *l_part, *l_joined = NULL, *l_block;
	int32_t l_pair[4];
	int32_t n = 0, l_count = 0, l_total = 0, i, l_position;

	printf("results in blocks\n");
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	for (i = 0; i < 4; i++) {
		l_part = sts_doc_spans(l_doc, l_ranges[2 * i], l_ranges[2 * i + 1], &l_count);
		l_joined = (int32_t *)realloc(l_joined, (size_t)(l_total + l_count + 1) * 4 * sizeof(int32_t));
		if (l_count > 0) {
			memcpy(l_joined + l_total * 4, l_part, (size_t)l_count * 4 * sizeof(int32_t));
		}
		l_total += l_count;
		sts_free(l_part);
	}
	l_all = sts_doc_spans_of(l_doc, l_ranges, 4, &n);
	check("the spans of four ranges are the four sets of spans, in order", l_total > 20 && same_ints(l_all, n, l_joined, l_total, 4));
	check("and they are well formed", spans_well_formed(l_doc, l_all, n));
	check("an ordinary result is not limited", sts_doc_was_limited(l_doc) == 0);
	sts_free(l_all);
	free(l_joined);
	l_all = sts_doc_spans_of(l_doc, l_ranges, 1, &n);
	l_part = sts_doc_spans(l_doc, 0, 4, &l_count);
	check("one range gives what sts_doc_spans gives", same_ints(l_all, n, l_part, l_count, 4));
	sts_free(l_all);
	sts_free(l_part);
	n = 7;
	check("no ranges give no spans", sts_doc_spans_of(l_doc, l_ranges, 0, &n) == NULL && n == 0);
	n = 7;
	check("no block of ranges gives no spans", sts_doc_spans_of(l_doc, NULL, 0, &n) == NULL && n == 0);
	check("no document gives no spans", sts_doc_spans_of(NULL, l_ranges, 4, &n) == NULL && n == 0);

	l_position = find(&l_text, "(a_name", 1);
	l_block = sts_doc_bracket_pair(l_doc, l_position);
	check("a pair in a block is the pair sts_doc_bracket_at gives", l_block && sts_doc_bracket_at(l_doc, l_position, l_pair) == 1
		&& memcmp(l_block, l_pair, sizeof l_pair) == 0 && l_block[0] == l_position && l_block[1] == 1);
	sts_free(l_block);
	check("no pair, no block", sts_doc_bracket_pair(l_doc, l_position + 3) == NULL);
	check("past the text, no block", sts_doc_bracket_pair(l_doc, l_text.count + 5) == NULL);
	check("no document, no block", sts_doc_bracket_pair(NULL, 0) == NULL);

	sts_doc_free(l_doc);
	free(l_text.chars);
}

static void test_other_languages(const char *a_fixtures)
{
	text32 l_text;
	sts_doc *l_doc;
	int32_t l_length = 0, l_position, l_pair[4];

	printf("json\n");
	l_text = read_text(a_fixtures, "sample.json");
	l_doc = sts_doc_new("json");
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	check("a key is a property", style_of(l_doc, &l_text, "\"count\"", 1, &l_length) == STS_STYLE_VARIABLE && l_length == 7);
	check("a value with wide characters is a string", style_of(l_doc, &l_text, "\"caf", 1, &l_length) == STS_STYLE_STRING && l_length == 10);
	check("42 is a number", style_of(l_doc, &l_text, "42", 1, &l_length) == STS_STYLE_NUMBER && l_length == 2);
	check("true is a number style", style_of(l_doc, &l_text, "true", 1, &l_length) == STS_STYLE_NUMBER);
	check("null is a constant", style_of(l_doc, &l_text, "null", 1, &l_length) == STS_STYLE_CONSTANT);
	check("the first brace finds the last", sts_doc_bracket_at(l_doc, 0, l_pair) == 1 && l_pair[0] == 0 && l_pair[2] == find(&l_text, "}\n", 2));
	l_position = find(&l_text, "[ \"a\"", 1);
	check("an inner bracket finds its own close", sts_doc_bracket_at(l_doc, l_position, l_pair) == 1 && l_pair[2] == l_position + 11);
	sts_doc_free(l_doc);
	free(l_text.chars);

	printf("xml\n");
	l_text = read_text(a_fixtures, "sample.ecf");
	l_doc = sts_doc_new("xml");
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	check("a tag name is a tag", style_of(l_doc, &l_text, "target", 1, &l_length) == STS_STYLE_TAG && l_length == 6);
	check("an attribute name is a property", style_of(l_doc, &l_text, "uuid", 1, &l_length) == STS_STYLE_VARIABLE && l_length == 4);
	check("a comment with wide characters", style_of(l_doc, &l_text, "<!-- A small", 1, &l_length) == STS_STYLE_COMMENT && l_length == 73);
	l_position = find(&l_text, "target", 1);
	check("an opening tag name finds the closing one", sts_doc_bracket_at(l_doc, l_position, l_pair) == 1 && l_pair[0] == l_position
		&& l_pair[1] == 6 && l_pair[2] == find(&l_text, "target", 2) && l_pair[3] == 6);
	l_position = find(&l_text, "<system", 1);
	check("a tag's angle brackets pair", sts_doc_bracket_at(l_doc, l_position, l_pair) == 1 && l_pair[0] == l_position
		&& l_pair[2] == find(&l_text, ">\n\t<!--", 1));
	sts_doc_free(l_doc);
	free(l_text.chars);

	printf("c\n");
	l_text = read_text(a_fixtures, "sample.c");
	l_doc = sts_doc_new("c");
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	check("static is a keyword", style_of(l_doc, &l_text, "static", 1, &l_length) == STS_STYLE_KEYWORD && l_length == 6);
	check("int is a type", style_of(l_doc, &l_text, "int total", 1, &l_length) == STS_STYLE_TYPE && l_length == 3);
	check("total is a function", style_of(l_doc, &l_text, "total(", 1, &l_length) == STS_STYLE_FUNCTION && l_length == 5);
	check("MAX_ITEMS is a constant, by its #match?", style_of(l_doc, &l_text, "MAX_ITEMS]", 1, &l_length) == STS_STYLE_CONSTANT && l_length == 9);
	check("values is a variable", style_of(l_doc, &l_text, "values[i] >", 1, &l_length) == STS_STYLE_VARIABLE && l_length == 6);
	check("a comment with wide characters", style_of(l_doc, &l_text, "/* A small", 1, &l_length) == STS_STYLE_COMMENT && l_length == find(&l_text, "\n", 1));
	l_position = find(&l_text, "{\n\tint i, sum", 1);
	check("a function's brace finds its close", sts_doc_bracket_at(l_doc, l_position, l_pair) == 1
		&& l_pair[2] == find(&l_text, "}\n\nint main", 1));
	sts_doc_free(l_doc);
	free(l_text.chars);
}

static int32_t run_count(const char *a_language, const char *a_text, const char *a_query, int32_t *an_error_kind)
	/* How many captures `a_query' gives over `a_text'; -1 when the query is refused. */
{
	text32 l_text = from_string(a_text);
	sts_doc *l_doc = sts_doc_new(a_language);
	char l_message[200];
	int32_t l_offset = 0, l_kind = 0, n = -1;
	sts_query *l_query = sts_query_new(a_language, a_query, (int32_t)strlen(a_query), &l_offset, &l_kind, l_message, sizeof(l_message));
	if (an_error_kind) {
		*an_error_kind = l_kind;
	}
	if (l_query) {
		sts_doc_set_text(l_doc, l_text.chars, l_text.count);
		sts_free(sts_doc_run(l_doc, l_query, 0, l_text.count - 1, &n));
		sts_query_free(l_query);
	}
	sts_doc_free(l_doc);
	free(l_text.chars);
	return n;
}

static int pattern_matches(const char *a_pattern, const char *a_subject)
	/* 1 when #match? `a_pattern' accepts `a_subject', 0 when not, -1 when the pattern is refused. */
{
	char l_json[400], l_query[400];
	snprintf(l_json, sizeof(l_json), "[\"%s\"]", a_subject);
	snprintf(l_query, sizeof(l_query), "((string_content) @s (#match? @s \"%s\"))", a_pattern);
	return (int)run_count("json", l_json, l_query, NULL);
}

static void test_queries(void)
{
	static const struct { const char *pattern, *subject; int expected; } l_cases[] = {
		{ "^[A-Z][A-Z\\\\d_]*$", "MAX_ITEMS", 1 },
		{ "^[A-Z][A-Z\\\\d_]*$", "MAX_items", 0 },
		{ "^[A-Z][A-Z\\\\d_]*$", "M", 1 },
		{ "abc", "xxabcxx", 1 },
		{ "abc", "xxabxcx", 0 },
		{ "^abc", "xabc", 0 },
		{ "abc$", "abcx", 0 },
		{ "^a.c$", "abc", 1 },
		{ "^a.c$", "ac", 0 },
		{ "^ab*c$", "ac", 1 },
		{ "^ab*c$", "abbbc", 1 },
		{ "^ab+c$", "ac", 0 },
		{ "^ab?c$", "abbc", 0 },
		{ "^(ab)+$", "ababab", 1 },
		{ "^(ab)+$", "ababa", 0 },
		{ "^(a|bc)+d$", "abcbcad", 1 },
		{ "^(a|bc)+d$", "abcbd", 0 },
		{ "^(?:get|set)_[a-z]+$", "set_name", 1 },
		{ "^(?:get|set)_[a-z]+$", "let_name", 0 },
		{ "^[^a-c]+$", "xyz", 1 },
		{ "^[^a-c]+$", "xbz", 0 },
		{ "^\\\\w+\\\\s\\\\d+$", "abc 123", 1 },
		{ "^\\\\w+\\\\s\\\\d+$", "abc  123", 0 },
		{ "^\\\\S+$", "a b", 0 },
		{ "^[\\\\w.]+$", "a.b_c", 1 },
		{ "^a\\\\.b$", "a.b", 1 },
		{ "^a\\\\.b$", "axb", 0 },
		{ "^(a*)*b$", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaac", 0 },
		{ "^(a|b)*$", "", -2 },
		{ "^[a-]+$", "a-a", 1 },
		{ "^[]a]+$", "]a", 1 },
		{ "caf.$", "caf\xC3\xA9", 1 },
		{ "^.$", "\xF0\x9F\x98\x80", 1 },
		{ "a{2}", "aa", -1 },
		{ "(?=a)a", "a", -1 },
		{ "a*?", "a", -1 },
		{ "\\\\bword", "word", -1 },
		{ "[[:alpha:]]", "a", -1 },
		{ "(ab", "ab", -1 },
		{ "ab)", "ab", -1 },
		{ "*a", "a", -1 }
	};
	static const char l_c[] = "int alpha = 1; int BETA = 2; int beta = BETA + alpha + alpha;";
	char l_message[200];
	int32_t l_offset = -1, l_kind = -1, n = 0;
	size_t i;
	sts_query *l_query;
	sts_doc *l_doc;
	text32 l_text;
	int32_t *l_captures;

	printf("queries\n");
	for (i = 0; i < sizeof(l_cases) / sizeof(l_cases[0]); i++) {
		char l_name[200];
		int l_result;
		if (l_cases[i].expected == -2) {
			continue;	/* an empty JSON string has no content node to test with */
		}
		l_result = pattern_matches(l_cases[i].pattern, l_cases[i].subject);
		snprintf(l_name, sizeof(l_name), "#match? %s on %s gives %d (got %d)", l_cases[i].pattern, l_cases[i].subject, l_cases[i].expected, l_result);
		check(l_name, l_result == l_cases[i].expected);
	}

	check("no predicate: every identifier", run_count("c", l_c, "(identifier) @i", NULL) == 6);
	check("#eq? with a string", run_count("c", l_c, "((identifier) @i (#eq? @i \"alpha\"))", NULL) == 3);
	check("#not-eq? with a string", run_count("c", l_c, "((identifier) @i (#not-eq? @i \"alpha\"))", NULL) == 3);
	check("#any-of?", run_count("c", l_c, "((identifier) @i (#any-of? @i \"alpha\" \"beta\"))", NULL) == 4);
	check("#not-any-of?", run_count("c", l_c, "((identifier) @i (#not-any-of? @i \"alpha\" \"beta\"))", NULL) == 2);
	check("#match?", run_count("c", l_c, "((identifier) @i (#match? @i \"^[A-Z]+$\"))", NULL) == 2);
	check("#not-match?", run_count("c", l_c, "((identifier) @i (#not-match? @i \"^[A-Z]+$\"))", NULL) == 4);
	check("#eq? with two captures", run_count("c", l_c,
		"((binary_expression left: (binary_expression right: (identifier) @a) right: (identifier) @b) (#eq? @a @b))", NULL) == 2);
	check("#not-eq? with two captures", run_count("c", "int x = a + b + c;",
		"((binary_expression left: (binary_expression right: (identifier) @a) right: (identifier) @b) (#not-eq? @a @b))", NULL) == 2);
	check("a directive is let through", run_count("c", l_c, "((identifier) @i (#set! \"priority\" 1))", NULL) == 6);

	check("a syntax error is refused", run_count("c", l_c, "(identifier @i", &l_kind) == -1 && l_kind == STS_QUERY_SYNTAX);
	check("an unknown node type is refused", run_count("c", l_c, "(no_such_node) @i", &l_kind) == -1 && l_kind == STS_QUERY_NODE_TYPE);
	check("an unknown field is refused", run_count("c", l_c, "(call_expression no_such_field: (identifier) @i)", &l_kind) == -1 && l_kind == STS_QUERY_FIELD);
	check("an unknown capture in a predicate is refused", run_count("c", l_c, "((identifier) @i (#eq? @j \"a\"))", &l_kind) == -1 && l_kind == STS_QUERY_CAPTURE);
	check("an unknown predicate is refused", run_count("c", l_c, "((identifier) @i (#frobnicate? @i \"a\"))", &l_kind) == -1 && l_kind == STS_QUERY_PREDICATE);
	check("a predicate without arguments is refused", run_count("c", l_c, "((identifier) @i (#eq? @i))", &l_kind) == -1 && l_kind == STS_QUERY_PREDICATE);
	check("an unsupported pattern is refused", run_count("c", l_c, "((identifier) @i (#match? @i \"a{2,3}\"))", &l_kind) == -1 && l_kind == STS_QUERY_PREDICATE);

	l_query = sts_query_new("c", "(identifier) @i\n(no_such_node) @j", 32, &l_offset, &l_kind, l_message, sizeof(l_message));
	check("the error says where and what", l_query == NULL && l_offset == 17 && l_kind == STS_QUERY_NODE_TYPE && strlen(l_message) > 5);
	l_query = sts_query_new("cobol", "(identifier) @i", 15, &l_offset, &l_kind, l_message, sizeof(l_message));
	check("an unknown language is refused", l_query == NULL && l_kind == STS_QUERY_LANGUAGE);
	l_query = sts_query_new("c", "((identifier) @i (#frobnicate? @i))", 35, &l_offset, &l_kind, l_message, 12);
	check("a short message buffer is respected", l_query == NULL && strlen(l_message) == 11);

	/* Captures: pattern, capture, start, length; and a range. */
	l_query = sts_query_new("c", "(number_literal) @n\n(identifier) @name", 38, &l_offset, &l_kind, l_message, sizeof(l_message));
	check("two captures", l_query != NULL && sts_query_capture_count(l_query) == 2 && sts_query_pattern_count(l_query) == 2);
	check("capture names", l_query && strcmp(sts_query_capture_name(l_query, 0), "n") == 0 && strcmp(sts_query_capture_name(l_query, 1), "name") == 0
		&& sts_query_capture_name(l_query, 2) == NULL);
	l_text = from_string(l_c);
	l_doc = sts_doc_new("c");
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	l_captures = sts_doc_run(l_doc, l_query, 0, 13, &n);
	check("captures in a range, in order", n == 2 && l_captures[0] == 1 && l_captures[1] == 1 && l_captures[2] == 4 && l_captures[3] == 5
		&& l_captures[4] == 0 && l_captures[5] == 0 && l_captures[6] == 12 && l_captures[7] == 1);
	sts_free(l_captures);
	check("an empty range gives nothing", sts_doc_run(l_doc, l_query, 5, 4, &n) == NULL && n == 0);
	sts_query_free(l_query);
	sts_doc_free(l_doc);
	free(l_text.chars);
}

static sts_doc *json_lines(text32 *a_text)
	/* A JSON document of twelve lines, a number on each of lines 1 to 10; nothing owed. */
{
	sts_doc *l_doc = sts_doc_new("json");
	*a_text = from_string("[\n 10,\n 11,\n 12,\n 13,\n 14,\n 15,\n 16,\n 17,\n 18,\n 19\n]");
	sts_doc_set_text(l_doc, a_text->chars, a_text->count);
	forget_changes(l_doc);
	return l_doc;
}

static void test_owed_lines(void)
	/* A changed line stays owed until sts_doc_changes_taken, and moves with the text. */
{
	text32 l_text;
	sts_doc *l_doc;
	int32_t *l_changed;
	int32_t n = 0;

	printf("owed lines\n");
	l_doc = json_lines(&l_text);
	check("twelve lines, nothing owed", sts_doc_line_count(l_doc) == 12 && sts_doc_changed_lines(l_doc, &n) == NULL && n == 0);

	/* Reported, then moved by a later edit, then asked again without a take. */
	edit_ascii(l_doc, &l_text, find(&l_text, "16", 1), 2, "66");
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("an edit on line 7 owes line 7 only", n == 1 && l_changed && l_changed[0] == 7 && l_changed[1] == 7);
	sts_free(l_changed);
	edit_ascii(l_doc, &l_text, find(&l_text, "11", 1), 2, "21,\n 22,\n 23");
	check("two lines added", sts_doc_line_count(l_doc) == 14);
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("not taken: the reported line is owed still, two lines lower, with the new edit's lines",
		n == 2 && l_changed && l_changed[0] == 2 && l_changed[1] == 4 && l_changed[2] == 9 && l_changed[3] == 9);
	sts_free(l_changed);
	sts_doc_changes_taken(l_doc);
	check("taken: nothing owed", sts_doc_changed_lines(l_doc, &n) == NULL && n == 0);

	/* An edit between the answer and the take stays owed; what was answered is forgotten. */
	edit_ascii(l_doc, &l_text, find(&l_text, "18", 1), 2, "88");
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("line 11 owed", n == 1 && l_changed && l_changed[0] == 11 && l_changed[1] == 11);
	sts_free(l_changed);
	edit_ascii(l_doc, &l_text, find(&l_text, "12", 1), 2, "32");
	sts_doc_changes_taken(l_doc);
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("the edit made before the take is owed after it, and only that", n == 1 && l_changed && l_changed[0] == 5 && l_changed[1] == 5);
	sts_free(l_changed);
	sts_doc_changes_taken(l_doc);

	/* Never reported: an owed line below an edit that removes lines moves up. */
	edit_ascii(l_doc, &l_text, find(&l_text, "19", 1), 2, "99");
	edit_ascii(l_doc, &l_text, find(&l_text, "21", 1), 12, "11");
	check("two lines removed", sts_doc_line_count(l_doc) == 12);
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("the owed line moved up two", n == 2 && l_changed && l_changed[0] == 2 && l_changed[1] == 2 && l_changed[2] == 10 && l_changed[3] == 10);
	sts_free(l_changed);

	/* A new text owes everything and drops what was owed before. */
	sts_doc_set_text(l_doc, l_text.chars, 9);
	l_changed = sts_doc_changed_lines(l_doc, &n);
	check("a new text owes all of itself and nothing else", n == 1 && l_changed && l_changed[0] == 0 && l_changed[1] == sts_doc_line_count(l_doc) - 1);
	sts_free(l_changed);
	sts_doc_changes_taken(NULL);
	sts_doc_free(l_doc);
	free(l_text.chars);
}

static void test_limits_and_wide_predicates(void)
{
	static const char l_json[] = "[\"na\xC3\xAFve \xE2\x88\x80x \xF0\x9F\x98\x80 done\", \"naive\", \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaac\"]";
	static const char l_slow[] = "((string_content) @s (#match? @s \"^(a*)*b$\"))";
	static const char l_plain[] = "(string_content) @s";
	char l_message[200];
	int32_t l_offset = 0, l_kind = 0, n = 0;
	text32 l_text = from_string(l_json);
	sts_doc *l_doc = sts_doc_new("json");
	sts_query *l_query;
	uint32_t *l_block;

	printf("limits and wide predicates\n");
	/* Predicate strings are UTF-8 and the text is UTF-32: they are compared as code points. */
	check("#eq? with accented, symbol and emoji characters",
		run_count("json", l_json, "((string_content) @s (#eq? @s \"na\xC3\xAFve \xE2\x88\x80x \xF0\x9F\x98\x80 done\"))", NULL) == 1);
	check("#eq? fails with one character changed",
		run_count("json", l_json, "((string_content) @s (#eq? @s \"na\xC3\xAFve \xE2\x88\x83x \xF0\x9F\x98\x80 done\"))", NULL) == 0);
	check("#eq? does not take the plain letter for the accented one",
		run_count("json", l_json, "((string_content) @s (#eq? @s \"naive \xE2\x88\x80x \xF0\x9F\x98\x80 done\"))", NULL) == 0);
	check("#not-eq? with wide characters",
		run_count("json", l_json, "((string_content) @s (#not-eq? @s \"na\xC3\xAFve \xE2\x88\x80x \xF0\x9F\x98\x80 done\"))", NULL) == 2);
	check("#any-of? with wide characters",
		run_count("json", l_json, "((string_content) @s (#any-of? @s \"naive\" \"na\xC3\xAFve \xE2\x88\x80x \xF0\x9F\x98\x80 done\"))", NULL) == 2);
	check("#match? with a wide character in a class",
		run_count("json", l_json, "((string_content) @s (#match? @s \"^na[\xC3\xAEi\xC3\xAF]ve\"))", NULL) == 2);

	/* A pattern that backtracks without end gives up, and the document says so. */
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	check("not limited before any result", sts_doc_was_limited(l_doc) == 0 && sts_doc_was_limited(NULL) == 0);
	l_query = sts_query_new("json", l_slow, (int32_t)strlen(l_slow), &l_offset, &l_kind, l_message, sizeof(l_message));
	check("the slow pattern compiles", l_query != NULL);
	check("it matches nothing", sts_doc_run(l_doc, l_query, 0, l_text.count - 1, &n) == NULL && n == 0);
	check("and the document says a limit was hit", sts_doc_was_limited(l_doc) == 1);
	sts_query_free(l_query);
	l_query = sts_query_new("json", l_plain, (int32_t)strlen(l_plain), &l_offset, &l_kind, l_message, sizeof(l_message));
	sts_free(sts_doc_run(l_doc, l_query, 0, l_text.count - 1, &n));
	check("an ordinary query after it is not limited", n == 3 && sts_doc_was_limited(l_doc) == 0);
	sts_query_free(l_query);
	sts_free(sts_doc_spans(l_doc, 0, 0, &n));
	check("ordinary spans are not limited", n > 0 && sts_doc_was_limited(l_doc) == 0);

	/* A text block for another thread: a counted copy. */
	l_block = sts_text_new(l_text.chars, l_text.count);
	check("a text block is a copy", l_block != NULL && l_block != l_text.chars
		&& memcmp(l_block, l_text.chars, (size_t)l_text.count * sizeof(uint32_t)) == 0);
	sts_doc_set_text(l_doc, l_block, l_text.count);
	sts_free(l_block);
	check("the document does not keep the block", sts_doc_count(l_doc) == l_text.count
		&& memcmp(sts_doc_text(l_doc), l_text.chars, (size_t)l_text.count * sizeof(uint32_t)) == 0);
	check("no block for no text", sts_text_new(l_text.chars, 0) == NULL && sts_text_new(NULL, 5) == NULL);
	sts_doc_free(l_doc);
	free(l_text.chars);
}

/* What an editor shows: for each line, the spans last applied to it. */
typedef struct {
	int32_t *items;	/* column, length, style per span */
	int32_t count;	/* of spans */
	int stale;	/* edited since its spans were applied */
} shown_line;

typedef struct {
	shown_line *lines;
	int32_t count;
} shown_text;

static void shown_edit(shown_text *a_shown, int32_t a_first, int32_t an_old_last, int32_t a_new_last)
	/* The lines `a_first' to `an_old_last' were edited into `a_first' to `a_new_last':
	   the lines after them keep what they show and move; the edited ones show nothing sure. */
{
	int32_t l_new_count = a_shown->count + (a_new_last - an_old_last), i;
	shown_line *l_lines = (shown_line *)calloc((size_t)l_new_count, sizeof(shown_line));
	for (i = 0; i < a_first; i++) {
		l_lines[i] = a_shown->lines[i];
	}
	for (i = a_first; i <= an_old_last; i++) {
		free(a_shown->lines[i].items);
	}
	for (i = a_first; i <= a_new_last; i++) {
		l_lines[i].stale = 1;
	}
	for (i = an_old_last + 1; i < a_shown->count; i++) {
		l_lines[i + (a_new_last - an_old_last)] = a_shown->lines[i];
	}
	free(a_shown->lines);
	a_shown->lines = l_lines;
	a_shown->count = l_new_count;
}

static void shown_apply(shown_text *a_shown, const int32_t *a_ranges, int32_t a_range_count, sts_doc *a_doc)
	/* Recolor the lines of `a_ranges' from `a_doc', as an editor does with a result. */
{
	int32_t r, i, n;
	for (r = 0; r < a_range_count; r++) {
		int32_t l_first = a_ranges[2 * r], l_last = a_ranges[2 * r + 1];
		int32_t *l_spans = sts_doc_spans(a_doc, l_first, l_last, &n);
		for (i = l_first; i <= l_last && i < a_shown->count; i++) {
			free(a_shown->lines[i].items);
			a_shown->lines[i].items = NULL;
			a_shown->lines[i].count = 0;
			a_shown->lines[i].stale = 0;
		}
		for (i = 0; i < n; i++) {
			shown_line *l_line = &a_shown->lines[l_spans[4 * i]];
			l_line->items = (int32_t *)realloc(l_line->items, ((size_t)l_line->count + 1) * 3 * sizeof(int32_t));
			memcpy(l_line->items + 3 * l_line->count, l_spans + 4 * i + 1, 3 * sizeof(int32_t));
			l_line->count++;
		}
		sts_free(l_spans);
	}
}

static int shown_is_fresh(const shown_text *a_shown, const char *a_language, const text32 *a_text, const char *a_what)
	/* Does every line show what a new document with `a_text' gives it? */
{
	sts_doc *l_fresh = sts_doc_new(a_language);
	int32_t *l_spans;
	int32_t n = 0, i, l_at = 0, l_line;
	int l_ok;
	sts_doc_set_text(l_fresh, a_text->chars, a_text->count);
	l_ok = sts_doc_line_count(l_fresh) == a_shown->count;
	if (!l_ok) {
		printf("  %s: %d lines shown, %d in the text\n", a_what, (int)a_shown->count, (int)sts_doc_line_count(l_fresh));
	}
	l_spans = sts_doc_spans(l_fresh, 0, sts_doc_line_count(l_fresh) - 1, &n);
	for (l_line = 0; l_ok && l_line < a_shown->count; l_line++) {
		const shown_line *l_shown = &a_shown->lines[l_line];
		int32_t l_count = 0;
		while (l_at + l_count < n && l_spans[4 * (l_at + l_count)] == l_line) {
			l_count++;
		}
		if (l_shown->stale) {
			printf("  %s: line %d was edited and never recolored\n", a_what, (int)l_line);
			l_ok = 0;
		} else if (l_shown->count != l_count) {
			printf("  %s: line %d shows %d spans, a fresh parse gives %d\n", a_what, (int)l_line, (int)l_shown->count, (int)l_count);
			l_ok = 0;
		} else {
			for (i = 0; i < l_count && l_ok; i++) {
				if (memcmp(l_shown->items + 3 * i, l_spans + 4 * (l_at + i) + 1, 3 * sizeof(int32_t)) != 0) {
					printf("  %s: line %d span %d differs from a fresh parse\n", a_what, (int)l_line, (int)i);
					l_ok = 0;
				}
			}
		}
		l_at += l_count;
	}
	sts_free(l_spans);
	sts_doc_free(l_fresh);
	return l_ok;
}

static void test_convergence(const char *a_fixtures, const char *a_language, const char *a_file)
	/* An editor that recolors only the lines it is told to, takes its results at random
	   moments and drops some of them, shows what a fresh parse gives once it has caught up. */
{
	text32 l_original = read_text(a_fixtures, a_file);
	text32 l_text = read_text(a_fixtures, a_file);
	sts_doc *l_doc = sts_doc_new(a_language);
	shown_text l_shown;
	int32_t *l_changed;
	int32_t n = 0, i;
	int l_round, l_ok = 1, l_dropped = 0, l_applied = 0;
	char l_what[120];

	printf("convergence %s\n", a_file);
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	l_shown.count = sts_doc_line_count(l_doc);
	l_shown.lines = (shown_line *)calloc((size_t)l_shown.count, sizeof(shown_line));
	for (i = 0; i < l_shown.count; i++) {
		l_shown.lines[i].stale = 1;
	}
	for (l_round = 0; l_round < 400 && l_ok; l_round++) {
		int32_t l_start = random_below(l_text.count + 1);
		int32_t l_removed = random_below(4) == 0 ? random_below(60) : random_below(6);
		int32_t l_inserted = random_below(4) == 0 ? random_below(60) : random_below(6);
		int32_t l_from = random_below(l_original.count - 60);
		int32_t l_first, l_old_last, l_new_last, l_column;
		int l_what_now = random_below(4);
		if (l_removed > l_text.count - l_start) {
			l_removed = l_text.count - l_start;
		}
		line_and_column(&l_text, l_start, &l_first, &l_column);
		line_and_column(&l_text, l_start + l_removed, &l_old_last, &l_column);
		edit_both(l_doc, &l_text, l_start, l_removed, l_original.chars + l_from, l_inserted);
		line_and_column(&l_text, l_start + l_inserted, &l_new_last, &l_column);
		shown_edit(&l_shown, l_first, l_old_last, l_new_last);
		if (l_what_now == 0) {
			/* A result taken and applied. */
			l_changed = sts_doc_changed_lines(l_doc, &n);
			shown_apply(&l_shown, l_changed, n, l_doc);
			sts_free(l_changed);
			sts_doc_changes_taken(l_doc);
			l_applied++;
			snprintf(l_what, sizeof(l_what), "%s round %d", a_file, l_round);
			l_ok = shown_is_fresh(&l_shown, a_language, &l_text, l_what);
		} else if (l_what_now == 1) {
			/* A result computed and then dropped as stale: nothing applied, nothing taken. */
			sts_free(sts_doc_changed_lines(l_doc, &n));
			l_dropped++;
		}
	}
	l_changed = sts_doc_changed_lines(l_doc, &n);
	shown_apply(&l_shown, l_changed, n, l_doc);
	sts_free(l_changed);
	sts_doc_changes_taken(l_doc);
	snprintf(l_what, sizeof(l_what), "%s at the end", a_file);
	check("results were dropped and applied along the way", l_dropped > 50 && l_applied > 50);
	check("each applied result showed what a fresh parse gives", l_ok);
	check("once caught up, the lines show what a fresh parse gives", l_ok && shown_is_fresh(&l_shown, a_language, &l_text, l_what));
	check("and nothing is owed", sts_doc_changed_lines(l_doc, &n) == NULL && n == 0);
	for (i = 0; i < l_shown.count; i++) {
		free(l_shown.lines[i].items);
	}
	free(l_shown.lines);
	sts_doc_free(l_doc);
	free(l_text.chars);
	free(l_original.chars);
}

static void test_trees(const char *a_fixtures)
{
	text32 l_text = read_text(a_fixtures, "sample.json");
	sts_doc *l_doc = sts_doc_new("json");
	sts_cursor *l_cursor;
	int32_t l_info[7], l_nodes = 0, l_depth = 0, l_done = 0;
	const char *l_type;
	char *l_sexp;

	printf("trees\n");
	check("no cursor before a document", sts_cursor_new(NULL) == NULL);
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	l_sexp = sts_doc_sexp(l_doc);
	check("the tree starts with the document", l_sexp && strncmp(l_sexp, "(document (object (pair key: (string", 35) == 0);
	sts_free(l_sexp);
	l_cursor = sts_cursor_new(l_doc);
	l_type = sts_cursor_node(l_cursor, l_info);
	check("the root covers the text", l_type && strcmp(l_type, "document") == 0 && l_info[0] == 0 && l_info[1] >= l_text.count - 1 && l_info[2] == 0);
	while (!l_done) {
		l_nodes++;
		if (sts_cursor_goto_first_child(l_cursor)) {
			l_depth++;
		} else {
			while (!l_done && !sts_cursor_goto_next_sibling(l_cursor)) {
				if (sts_cursor_goto_parent(l_cursor)) {
					l_depth--;
				} else {
					l_done = 1;
				}
			}
		}
	}
	check("a walk visits every node and comes back", l_nodes > 40 && l_depth == 0);
	sts_cursor_free(l_cursor);
	sts_doc_free(l_doc);
	free(l_text.chars);
}

static void test_speed(const char *a_fixtures)
	/* Not a check: how long one large Eiffel class takes (the fixture's features, many times). */
{
	text32 l_one = read_text(a_fixtures, "sample.e");
	int32_t l_copies = 200, i, n = 0, l_position, l_count = 0;
	int32_t l_head = find(&l_one, "feature {NONE}", 1), l_tail = find(&l_one, "invariant", 1);
	int32_t l_body = l_tail - l_head;
	uint32_t *l_big = (uint32_t *)malloc(((size_t)l_one.count + (size_t)l_body * (size_t)l_copies) * sizeof(uint32_t));
	sts_doc *l_doc = sts_doc_new("eiffel");
	uint32_t l_x = 'x';
	clock_t l_start;
	double l_parse, l_edit, l_spans;

	memcpy(l_big, l_one.chars, (size_t)l_head * sizeof(uint32_t));
	l_count = l_head;
	for (i = 0; i < l_copies; i++) {
		memcpy(l_big + l_count, l_one.chars + l_head, (size_t)l_body * sizeof(uint32_t));
		l_count += l_body;
	}
	memcpy(l_big + l_count, l_one.chars + l_tail, (size_t)(l_one.count - l_tail) * sizeof(uint32_t));
	l_count += l_one.count - l_tail;
	l_start = clock();
	sts_doc_set_text(l_doc, l_big, l_count);
	l_parse = (double)(clock() - l_start) * 1000.0 / CLOCKS_PER_SEC;
	check("the large class parses without errors", sts_doc_has_error(l_doc) == 0);
	l_position = l_head + l_body * (l_copies / 2) + (find(&l_one, "greeting :=", 1) - l_head);
	l_start = clock();
	for (i = 0; i < 100; i++) {
		sts_doc_edit(l_doc, l_position, 0, &l_x, 1);
		forget_changes(l_doc);
	}
	l_edit = (double)(clock() - l_start) * 1000.0 / CLOCKS_PER_SEC / 100.0;
	l_start = clock();
	for (i = 0; i < 100; i++) {
		sts_free(sts_doc_spans(l_doc, 7000, 7060, &n));
	}
	l_spans = (double)(clock() - l_start) * 1000.0 / CLOCKS_PER_SEC / 100.0;
	printf("speed: %d lines, %d characters: first parse %.1f ms, one typed character %.3f ms, spans for 60 lines %.3f ms\n",
		(int)sts_doc_line_count(l_doc), (int)sts_doc_count(l_doc), l_parse, l_edit, l_spans);
	sts_doc_free(l_doc);
	free(l_big);
	free(l_one.chars);
}

static int survey(int argc, char **argv)
	/* For each file with errors: its lines, its error nodes, how many of its lines lie in
	   an error node, and the lines the first errors are on. Then the totals. */
{
	int i, l_files = 0, l_clean = 0;
	long l_total_lines = 0, l_total_errors = 0, l_total_bad = 0;
	for (i = 3; i < argc; i++) {
		text32 l_text = read_text(NULL, argv[i]);
		sts_doc *l_doc = sts_doc_new(argv[2]);
		sts_cursor *l_cursor;
		int32_t l_info[7], l_errors = 0, l_done = 0, l_first[5], l_shown = 0, k, l_lines, l_bad = 0;
		char *l_marks;
		if (!l_doc) {
			printf("unknown language %s\n", argv[2]);
			return 2;
		}
		sts_doc_set_text(l_doc, l_text.chars, l_text.count);
		l_lines = sts_doc_line_count(l_doc);
		l_marks = (char *)calloc((size_t)l_lines + 1, 1);
		l_cursor = sts_cursor_new(l_doc);
		while (l_cursor && !l_done) {
			sts_cursor_node(l_cursor, l_info);
			if (l_info[5] || l_info[6]) {
				if (l_shown < 5) {
					l_first[l_shown++] = l_info[2] + 1;
				}
				for (k = l_info[2]; k <= l_info[3] && k < l_lines; k++) {
					l_marks[k] = 1;
				}
				l_errors++;
			}
			if (!sts_cursor_goto_first_child(l_cursor)) {
				while (!l_done && !sts_cursor_goto_next_sibling(l_cursor)) {
					l_done = !sts_cursor_goto_parent(l_cursor);
				}
			}
		}
		sts_cursor_free(l_cursor);
		for (k = 0; k < l_lines; k++) {
			l_bad += l_marks[k];
		}
		free(l_marks);
		l_files++;
		l_clean += l_errors == 0;
		l_total_lines += l_lines;
		l_total_errors += l_errors;
		l_total_bad += l_bad;
		if (l_errors) {
			printf("%s: %d lines, %d error nodes over %d lines, first on lines", argv[i], (int)l_lines, (int)l_errors, (int)l_bad);
			for (k = 0; k < l_shown; k++) {
				printf(" %d", (int)l_first[k]);
			}
			printf("\n");
		}
		sts_doc_free(l_doc);
		free(l_text.chars);
	}
	printf("survey: %d files, %d without errors, %ld lines, %ld error nodes, %ld lines in error nodes\n",
		l_files, l_clean, l_total_lines, l_total_errors, l_total_bad);
	return 0;
}

static int dump(const char *a_language, const char *a_file)
	/* The tree, the spans and the folds of one file, for a look by eye. */
{
	text32 l_text = read_text(NULL, a_file);
	sts_doc *l_doc = sts_doc_new(a_language);
	int32_t *l_items;
	int32_t n = 0, i, k;
	char *l_sexp;
	if (!l_doc) {
		printf("unknown language %s\n", a_language);
		return 2;
	}
	sts_doc_set_text(l_doc, l_text.chars, l_text.count);
	l_sexp = sts_doc_sexp(l_doc);
	printf("%s\n", l_sexp ? l_sexp : "(no tree)");
	sts_free(l_sexp);
	l_items = sts_doc_spans(l_doc, 0, sts_doc_line_count(l_doc) - 1, &n);
	for (i = 0; i < n; i++) {
		int32_t l_start = sts_doc_line_start(l_doc, l_items[4 * i]) + l_items[4 * i + 1];
		printf("span line %d column %d length %d style %d: ", (int)l_items[4 * i] + 1, (int)l_items[4 * i + 1] + 1,
			(int)l_items[4 * i + 2], (int)l_items[4 * i + 3]);
		for (k = 0; k < l_items[4 * i + 2]; k++) {
			uint32_t c = l_text.chars[l_start + k];
			putchar(c < 128 ? (int)c : '?');
		}
		putchar('\n');
	}
	sts_free(l_items);
	l_items = sts_doc_folds(l_doc, &n);
	for (i = 0; i < n; i++) {
		printf("fold lines %d to %d\n", (int)l_items[2 * i] + 1, (int)l_items[2 * i + 1] + 1);
	}
	sts_free(l_items);
	sts_doc_free(l_doc);
	free(l_text.chars);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc == 4 && strcmp(argv[1], "--dump") == 0) {
		return dump(argv[2], argv[3]);
	}
	if (argc >= 4 && strcmp(argv[1], "--survey") == 0) {
		return survey(argc, argv);
	}
	if (argc != 3) {
		printf("usage: sts_test <fixtures directory> <queries directory>\n");
		return 2;
	}
	test_languages();
	test_bundled_queries(argv[2]);
	test_fixture(argv[1], "eiffel", "sample.e");
	test_fixture(argv[1], "c", "sample.c");
	test_fixture(argv[1], "json", "sample.json");
	test_fixture(argv[1], "xml", "sample.ecf");
	test_eiffel(argv[1]);
	test_fixture(argv[1], "eiffel", "constructs.e");
	test_eiffel_constructs(argv[1]);
	test_other_languages(argv[1]);
	test_queries();
	test_limits_and_wide_predicates();
	test_owed_lines();
	test_results_in_blocks(argv[1]);
	test_trees(argv[1]);
	test_random_edits(argv[1], "eiffel", "sample.e");
	test_random_edits(argv[1], "eiffel", "constructs.e");
	test_random_edits(argv[1], "c", "sample.c");
	test_random_edits(argv[1], "json", "sample.json");
	test_random_edits(argv[1], "xml", "sample.ecf");
	test_convergence(argv[1], "eiffel", "sample.e");
	test_convergence(argv[1], "c", "sample.c");
	test_convergence(argv[1], "json", "sample.json");
	test_convergence(argv[1], "xml", "sample.ecf");
	test_speed(argv[1]);
	check("nothing left allocated", sts_live_blocks() == 0);
	if (sts_live_blocks() != 0) {
		printf("  %d blocks still live\n", (int)sts_live_blocks());
	}
	printf("=== Results: %d passed, %d failed ===\n", s_checks - s_failures, s_failures);
	return s_failures ? 1 : 0;
}
