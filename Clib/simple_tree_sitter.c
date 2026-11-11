/*
 * simple_tree_sitter: the C side of the Eiffel library. See simple_tree_sitter.h.
 */

#include "simple_tree_sitter.h"
#include "tree_sitter/api.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Memory: every block and handle given out is counted, for the leak tests.   */
/* ------------------------------------------------------------------------- */

#ifdef _WIN32
#include <windows.h>
static volatile LONG s_live = 0;
#define STS_LIVE_ADD(n) InterlockedExchangeAdd(&s_live, (n))
#else
static volatile int32_t s_live = 0;
#define STS_LIVE_ADD(n) __sync_fetch_and_add(&s_live, (n))
#endif

static void *sts_alloc(size_t a_size)
{
	void *p = calloc(1, a_size ? a_size : 1);
	if (p) {
		STS_LIVE_ADD(1);
	}
	return p;
}

void sts_free(void *a_block)
{
	if (a_block) {
		STS_LIVE_ADD(-1);
		free(a_block);
	}
}

int32_t sts_live_blocks(void)
{
	return (int32_t)s_live;
}

/* A growing list of int32_t, handed out as a counted block. */
typedef struct {
	int32_t *items;
	int32_t count;
	int32_t capacity;
} sts_ints;

static void sts_ints_put(sts_ints *a_list, int32_t a_value)
{
	if (a_list->count == a_list->capacity) {
		int32_t l_capacity = a_list->capacity ? a_list->capacity * 2 : 64;
		int32_t *l_items = (int32_t *)realloc(a_list->items, (size_t)l_capacity * sizeof(int32_t));
		if (!l_items) {
			return;
		}
		a_list->items = l_items;
		a_list->capacity = l_capacity;
	}
	a_list->items[a_list->count++] = a_value;
}

static int32_t *sts_ints_take(sts_ints *a_list, int32_t a_per_item, int32_t *a_count)
	/* The list as a counted block (NULL when empty), and its item count. */
{
	int32_t *l_block = NULL;
	if (a_list->count > 0) {
		l_block = (int32_t *)sts_alloc((size_t)a_list->count * sizeof(int32_t));
		if (l_block) {
			memcpy(l_block, a_list->items, (size_t)a_list->count * sizeof(int32_t));
		}
	}
	if (a_count) {
		*a_count = l_block ? a_list->count / a_per_item : 0;
	}
	free(a_list->items);
	a_list->items = NULL;
	a_list->count = a_list->capacity = 0;
	return l_block;
}

/* ------------------------------------------------------------------------- */
/* Languages                                                                  */
/* ------------------------------------------------------------------------- */

extern const TSLanguage *tree_sitter_eiffel(void);
extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_json(void);
extern const TSLanguage *tree_sitter_xml(void);

/* From sts_queries.c (generated from queries/): kind 0 highlights, 1 folds, 2 brackets. */
extern const char *sts_bundled_query(const char *a_language, int a_kind);

typedef struct {
	const char *name;
	const TSLanguage *(*language)(void);
	const char *extensions;	/* each with a space before and after */
} sts_lang;

/* To add a grammar: one line here, its sources under grammars/, its queries under queries/. */
static const sts_lang s_languages[] = {
	{ "eiffel", tree_sitter_eiffel, " e " },
	{ "c", tree_sitter_c, " c h " },
	{ "json", tree_sitter_json, " json " },
	{ "xml", tree_sitter_xml, " xml ecf " }
};

#define STS_LANGUAGE_COUNT ((int32_t)(sizeof(s_languages) / sizeof(s_languages[0])))

static const sts_lang *sts_lang_named(const char *a_name)
{
	int32_t i;
	if (a_name) {
		for (i = 0; i < STS_LANGUAGE_COUNT; i++) {
			if (strcmp(s_languages[i].name, a_name) == 0) {
				return &s_languages[i];
			}
		}
	}
	return NULL;
}

int32_t sts_language_count(void)
{
	return STS_LANGUAGE_COUNT;
}

const char *sts_language_name(int32_t a_index)
{
	return (a_index >= 0 && a_index < STS_LANGUAGE_COUNT) ? s_languages[a_index].name : NULL;
}

int32_t sts_language_has(const char *a_name)
{
	return sts_lang_named(a_name) != NULL;
}

const char *sts_language_for_extension(const char *an_extension)
{
	char l_key[20];
	size_t n, i;
	int32_t k;
	if (!an_extension) {
		return NULL;
	}
	n = strlen(an_extension);
	if (n == 0 || n > sizeof(l_key) - 3) {
		return NULL;
	}
	l_key[0] = ' ';
	for (i = 0; i < n; i++) {
		char c = an_extension[i];
		if (c == ' ') {
			return NULL;
		}
		l_key[i + 1] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
	}
	l_key[n + 1] = ' ';
	l_key[n + 2] = 0;
	for (k = 0; k < STS_LANGUAGE_COUNT; k++) {
		if (strstr(s_languages[k].extensions, l_key)) {
			return s_languages[k].name;
		}
	}
	return NULL;
}

int32_t sts_style_for_capture(const char *a_name)
{
	static const struct { const char *name; int32_t style; } l_table[] = {
		{ "keyword", STS_STYLE_KEYWORD },
		{ "type", STS_STYLE_TYPE },
		{ "string", STS_STYLE_STRING },
		{ "character", STS_STYLE_STRING },
		{ "comment", STS_STYLE_COMMENT },
		{ "number", STS_STYLE_NUMBER },
		{ "float", STS_STYLE_NUMBER },
		{ "boolean", STS_STYLE_NUMBER },
		{ "operator", STS_STYLE_OPERATOR },
		{ "punctuation", STS_STYLE_OPERATOR },
		{ "function", STS_STYLE_FUNCTION },
		{ "method", STS_STYLE_FUNCTION },
		{ "constructor", STS_STYLE_FUNCTION },
		{ "constant", STS_STYLE_CONSTANT },
		{ "variable", STS_STYLE_VARIABLE },
		{ "property", STS_STYLE_VARIABLE },
		{ "parameter", STS_STYLE_VARIABLE },
		{ "field", STS_STYLE_VARIABLE },
		{ "tag", STS_STYLE_TAG },
		{ "attribute", STS_STYLE_TAG }
	};
	size_t l_first, i;
	const char *l_dot;
	if (!a_name) {
		return STS_STYLE_PLAIN;
	}
	l_dot = strchr(a_name, '.');
	l_first = l_dot ? (size_t)(l_dot - a_name) : strlen(a_name);
	for (i = 0; i < sizeof(l_table) / sizeof(l_table[0]); i++) {
		if (strlen(l_table[i].name) == l_first && strncmp(l_table[i].name, a_name, l_first) == 0) {
			return l_table[i].style;
		}
	}
	return STS_STYLE_PLAIN;
}

/* ------------------------------------------------------------------------- */
/* UTF-8 (query sources) to code points                                       */
/* ------------------------------------------------------------------------- */

static uint32_t *sts_utf8_to_32(const char *a_text, uint32_t a_length, int32_t *a_count)
	/* A plain malloc block (never empty) with the code points of `a_text'. */
{
	const uint8_t *s = (const uint8_t *)a_text;
	uint32_t *l_out = (uint32_t *)malloc(((size_t)a_length + 1) * sizeof(uint32_t));
	uint32_t i = 0;
	int32_t n = 0;
	if (!l_out) {
		*a_count = 0;
		return NULL;
	}
	while (i < a_length) {
		uint32_t c = s[i];
		uint32_t l_more = 0;
		if (c < 0x80) {
			l_more = 0;
		} else if ((c & 0xE0) == 0xC0) {
			c &= 0x1F; l_more = 1;
		} else if ((c & 0xF0) == 0xE0) {
			c &= 0x0F; l_more = 2;
		} else if ((c & 0xF8) == 0xF0) {
			c &= 0x07; l_more = 3;
		} else {
			c = 0xFFFD;
		}
		i++;
		while (l_more > 0 && i < a_length && (s[i] & 0xC0) == 0x80) {
			c = (c << 6) | (s[i] & 0x3F);
			i++;
			l_more--;
		}
		if (l_more > 0) {
			c = 0xFFFD;
		}
		l_out[n++] = c;
	}
	*a_count = n;
	return l_out;
}

/* ------------------------------------------------------------------------- */
/* #match? patterns: a small matcher, the same on every platform.             */
/*   ^ $ . [classes] \d \w \s \D \W \S * + ? (groups) |                       */
/* ------------------------------------------------------------------------- */

enum { RX_CHAR, RX_ANY, RX_CLASS, RX_BOL, RX_EOL, RX_GROUP };
enum { RX_DIGIT = 1, RX_WORD = 2, RX_SPACE = 4, RX_NOT_DIGIT = 8, RX_NOT_WORD = 16, RX_NOT_SPACE = 32 };

#define RX_MAX_STEPS 200000
#define RX_MAX_DEPTH 2000

typedef struct rx_node rx_node;
typedef struct rx_alt rx_alt;

struct rx_alt {
	rx_node *first;
	rx_alt *next;
};

struct rx_node {
	int kind;
	uint32_t ch;
	uint32_t *ranges;	/* pairs: low, high */
	int range_count;
	int negate;
	int flags;
	rx_alt *alts;
	int min, max;	/* max < 0: no upper limit */
	rx_node *next;
};

typedef struct rx_piece {
	struct rx_piece *next;
} rx_piece;

typedef struct {
	rx_node *top;
	rx_piece *pieces;	/* every allocation, for freeing */
} sts_rx;

typedef struct {
	sts_rx *rx;
	const uint32_t *p;
	int pos, len;
	int failed;
	int depth;
} rx_parser;

static void *rx_new(sts_rx *a_rx, size_t a_size)
{
	rx_piece *l_piece = (rx_piece *)calloc(1, sizeof(rx_piece) + a_size);
	if (!l_piece) {
		return NULL;
	}
	l_piece->next = a_rx->pieces;
	a_rx->pieces = l_piece;
	return (void *)(l_piece + 1);
}

static void rx_free(sts_rx *a_rx)
{
	if (a_rx) {
		rx_piece *l_piece = a_rx->pieces;
		while (l_piece) {
			rx_piece *l_next = l_piece->next;
			free(l_piece);
			l_piece = l_next;
		}
		free(a_rx);
	}
}

static rx_alt *rx_parse_alts(rx_parser *a);

static int rx_escape_flag(uint32_t c)
{
	switch (c) {
	case 'd': return RX_DIGIT;
	case 'w': return RX_WORD;
	case 's': return RX_SPACE;
	case 'D': return RX_NOT_DIGIT;
	case 'W': return RX_NOT_WORD;
	case 'S': return RX_NOT_SPACE;
	default: return 0;
	}
}

static uint32_t rx_escape_char(rx_parser *a, uint32_t c)
	/* The character an escaped `c' stands for; unsupported escapes fail. */
{
	switch (c) {
	case 'n': return '\n';
	case 't': return '\t';
	case 'r': return '\r';
	case 'f': return '\f';
	case 'v': return '\v';
	default:
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
			a->failed = 1;	/* \b, \B, \1, \p{...}, \x41 and the like */
		}
		return c;
	}
}

static void rx_parse_class(rx_parser *a, rx_node *a_node)
{
	int l_capacity = 8, l_first = 1;
	uint32_t *l_ranges = (uint32_t *)rx_new(a->rx, sizeof(uint32_t) * 2 * (size_t)l_capacity);
	a_node->kind = RX_CLASS;
	if (a->pos < a->len && a->p[a->pos] == '^') {
		a_node->negate = 1;
		a->pos++;
	}
	while (!a->failed && l_ranges) {
		uint32_t l_low, l_high;
		if (a->pos >= a->len) {
			a->failed = 1;
			break;
		}
		l_low = a->p[a->pos++];
		if (l_low == ']' && !l_first) {
			break;
		}
		l_first = 0;
		if (l_low == '[' && a->pos < a->len && a->p[a->pos] == ':') {
			a->failed = 1;	/* [:alpha:] */
			break;
		}
		if (l_low == '\\') {
			int l_flag;
			if (a->pos >= a->len) {
				a->failed = 1;
				break;
			}
			l_low = a->p[a->pos++];
			l_flag = rx_escape_flag(l_low);
			if (l_flag) {
				a_node->flags |= l_flag;
				continue;
			}
			l_low = rx_escape_char(a, l_low);
		}
		l_high = l_low;
		if (a->pos + 1 < a->len && a->p[a->pos] == '-' && a->p[a->pos + 1] != ']') {
			a->pos++;
			l_high = a->p[a->pos++];
			if (l_high == '\\') {
				if (a->pos >= a->len) {
					a->failed = 1;
					break;
				}
				l_high = rx_escape_char(a, a->p[a->pos++]);
			}
			if (l_high < l_low) {
				a->failed = 1;
				break;
			}
		}
		if (a_node->range_count == l_capacity) {
			uint32_t *l_bigger = (uint32_t *)rx_new(a->rx, sizeof(uint32_t) * 4 * (size_t)l_capacity);
			if (!l_bigger) {
				a->failed = 1;
				break;
			}
			memcpy(l_bigger, l_ranges, sizeof(uint32_t) * 2 * (size_t)l_capacity);
			l_ranges = l_bigger;
			l_capacity *= 2;
		}
		l_ranges[2 * a_node->range_count] = l_low;
		l_ranges[2 * a_node->range_count + 1] = l_high;
		a_node->range_count++;
	}
	a_node->ranges = l_ranges;
}

static rx_node *rx_parse_atom(rx_parser *a)
{
	rx_node *l_node = (rx_node *)rx_new(a->rx, sizeof(rx_node));
	uint32_t c;
	if (!l_node) {
		a->failed = 1;
		return NULL;
	}
	l_node->min = l_node->max = 1;
	c = a->p[a->pos++];
	switch (c) {
	case '(':
		if (a->pos + 1 < a->len && a->p[a->pos] == '?') {
			if (a->p[a->pos + 1] == ':') {
				a->pos += 2;
			} else {
				a->failed = 1;	/* look-around, named groups, flags */
			}
		}
		l_node->kind = RX_GROUP;
		if (++a->depth > 100) {
			a->failed = 1;
		} else {
			l_node->alts = rx_parse_alts(a);
		}
		a->depth--;
		if (a->pos < a->len && a->p[a->pos] == ')') {
			a->pos++;
		} else {
			a->failed = 1;
		}
		break;
	case '[':
		rx_parse_class(a, l_node);
		break;
	case '.':
		l_node->kind = RX_ANY;
		break;
	case '^':
		l_node->kind = RX_BOL;
		break;
	case '$':
		l_node->kind = RX_EOL;
		break;
	case '*': case '+': case '?': case '{':
		a->failed = 1;	/* nothing to repeat; counted repeats are not supported */
		break;
	case '\\':
		if (a->pos >= a->len) {
			a->failed = 1;
		} else {
			int l_flag;
			c = a->p[a->pos++];
			l_flag = rx_escape_flag(c);
			if (l_flag) {
				l_node->kind = RX_CLASS;
				l_node->flags = l_flag;
			} else {
				l_node->kind = RX_CHAR;
				l_node->ch = rx_escape_char(a, c);
			}
		}
		break;
	default:
		l_node->kind = RX_CHAR;
		l_node->ch = c;
		break;
	}
	if (!a->failed && a->pos < a->len && l_node->kind != RX_BOL && l_node->kind != RX_EOL) {
		c = a->p[a->pos];
		if (c == '*' || c == '+' || c == '?') {
			a->pos++;
			l_node->min = (c == '+') ? 1 : 0;
			l_node->max = (c == '?') ? 1 : -1;
			if (a->pos < a->len && (a->p[a->pos] == '?' || a->p[a->pos] == '+')) {
				a->failed = 1;	/* lazy and possessive repeats */
			}
		} else if (c == '{') {
			a->failed = 1;
		}
	}
	return l_node;
}

static rx_alt *rx_parse_alts(rx_parser *a)
{
	rx_alt *l_first = NULL, *l_last = NULL;
	for (;;) {
		rx_alt *l_alt = (rx_alt *)rx_new(a->rx, sizeof(rx_alt));
		rx_node *l_tail = NULL;
		if (!l_alt) {
			a->failed = 1;
			break;
		}
		if (l_last) {
			l_last->next = l_alt;
		} else {
			l_first = l_alt;
		}
		l_last = l_alt;
		while (!a->failed && a->pos < a->len && a->p[a->pos] != '|' && a->p[a->pos] != ')') {
			rx_node *l_node = rx_parse_atom(a);
			if (l_node) {
				if (l_tail) {
					l_tail->next = l_node;
				} else {
					l_alt->first = l_node;
				}
				l_tail = l_node;
			}
		}
		if (!a->failed && a->pos < a->len && a->p[a->pos] == '|') {
			a->pos++;
		} else {
			break;
		}
	}
	return l_first;
}

static sts_rx *rx_compile(const uint32_t *a_pattern, int a_length)
	/* NULL when the pattern uses something outside the supported subset. */
{
	rx_parser l_parser;
	sts_rx *l_rx = (sts_rx *)calloc(1, sizeof(sts_rx));
	if (!l_rx) {
		return NULL;
	}
	memset(&l_parser, 0, sizeof(l_parser));
	l_parser.rx = l_rx;
	l_parser.p = a_pattern;
	l_parser.len = a_length;
	l_rx->top = (rx_node *)rx_new(l_rx, sizeof(rx_node));
	if (l_rx->top) {
		l_rx->top->kind = RX_GROUP;
		l_rx->top->min = l_rx->top->max = 1;
		l_rx->top->alts = rx_parse_alts(&l_parser);
	}
	if (!l_rx->top || l_parser.failed || l_parser.pos != l_parser.len) {
		rx_free(l_rx);
		return NULL;
	}
	return l_rx;
}

typedef struct rx_cont {
	const rx_node *group;	/* the group whose alternative just ended */
	int count;	/* its repeats, counting this one */
	int start;	/* where this repeat began */
	const struct rx_cont *up;
} rx_cont;

typedef struct {
	const uint32_t *s;
	int len;
	int steps;
	int depth;
	int limited;	/* a limit stopped the search somewhere */
} rx_run_state;

static int rx_is_word(uint32_t c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static int rx_is_space(uint32_t c)
{
	return c == ' ' || (c >= '\t' && c <= '\r');
}

static int rx_one(const rx_node *n, uint32_t c)
{
	int i, l_in = 0;
	switch (n->kind) {
	case RX_CHAR:
		return c == n->ch;
	case RX_ANY:
		return c != '\n';
	default:
		for (i = 0; i < n->range_count && !l_in; i++) {
			l_in = c >= n->ranges[2 * i] && c <= n->ranges[2 * i + 1];
		}
		if (!l_in && n->flags) {
			int l_digit = c >= '0' && c <= '9';
			l_in = ((n->flags & RX_DIGIT) && l_digit)
				|| ((n->flags & RX_NOT_DIGIT) && !l_digit)
				|| ((n->flags & RX_WORD) && rx_is_word(c))
				|| ((n->flags & RX_NOT_WORD) && !rx_is_word(c))
				|| ((n->flags & RX_SPACE) && rx_is_space(c))
				|| ((n->flags & RX_NOT_SPACE) && !rx_is_space(c));
		}
		return n->negate ? !l_in : l_in;
	}
}

static int rx_run(rx_run_state *a, const rx_node *n, const rx_cont *k, int a_pos);

static int rx_repeat(rx_run_state *a, const rx_node *a_group, int a_count, const rx_cont *k, int a_pos)
	/* `a_group' has matched `a_count' times and stands at `a_pos'. */
{
	int l_result = 0;
	if (++a->steps > RX_MAX_STEPS || a->depth >= RX_MAX_DEPTH) {
		a->limited = 1;
		return 0;
	}
	a->depth++;
	if (a_group->max < 0 || a_count < a_group->max) {
		rx_cont l_me;
		const rx_alt *l_alt;
		l_me.group = a_group;
		l_me.count = a_count + 1;
		l_me.start = a_pos;
		l_me.up = k;
		for (l_alt = a_group->alts; l_alt && !l_result; l_alt = l_alt->next) {
			l_result = rx_run(a, l_alt->first, &l_me, a_pos);
		}
	}
	if (!l_result && a_count >= a_group->min) {
		l_result = rx_run(a, a_group->next, k, a_pos);
	}
	a->depth--;
	return l_result;
}

static int rx_run(rx_run_state *a, const rx_node *n, const rx_cont *k, int a_pos)
{
	int l_result = 0;
	if (++a->steps > RX_MAX_STEPS || a->depth >= RX_MAX_DEPTH) {
		a->limited = 1;
		return 0;
	}
	a->depth++;
	if (!n) {
		if (!k) {
			l_result = 1;
		} else if (a_pos == k->start) {
			/* A repeat that took nothing: do not go round again. */
			l_result = rx_run(a, k->group->next, k->up, a_pos);
		} else {
			l_result = rx_repeat(a, k->group, k->count, k->up, a_pos);
		}
	} else {
		switch (n->kind) {
		case RX_BOL:
			l_result = a_pos == 0 && rx_run(a, n->next, k, a_pos);
			break;
		case RX_EOL:
			l_result = a_pos == a->len && rx_run(a, n->next, k, a_pos);
			break;
		case RX_GROUP:
			l_result = rx_repeat(a, n, 0, k, a_pos);
			break;
		default:
			{
				int l_count = 0;
				while ((n->max < 0 || l_count < n->max) && a_pos + l_count < a->len && rx_one(n, a->s[a_pos + l_count])) {
					l_count++;
				}
				for (; l_count >= n->min && !l_result; l_count--) {
					l_result = rx_run(a, n->next, k, a_pos + l_count);
				}
			}
			break;
		}
	}
	a->depth--;
	return l_result;
}

static int rx_search(const sts_rx *a_rx, const uint32_t *a_text, int a_length, int *a_limited)
	/* Does `a_rx' match anywhere in `a_text'? A search that a limit stopped answers no
	   and sets `a_limited'. */
{
	rx_run_state l_state;
	int i;
	l_state.s = a_text;
	l_state.len = a_length;
	l_state.steps = 0;
	l_state.depth = 0;
	l_state.limited = 0;
	for (i = 0; i <= a_length; i++) {
		if (rx_run(&l_state, a_rx->top, NULL, i)) {
			return 1;
		}
		if (l_state.limited) {
			*a_limited = 1;
			return 0;
		}
	}
	return 0;
}

/* ------------------------------------------------------------------------- */
/* Queries and their text predicates                                          */
/* ------------------------------------------------------------------------- */

enum { STS_PRED_EQ, STS_PRED_ANY_OF, STS_PRED_MATCH };

typedef struct {
	int kind;
	int negate;
	uint32_t capture;
	int32_t other_capture;	/* #eq? @a @b; else -1 */
	uint32_t **strings;
	int32_t *lengths;
	int32_t string_count;
	sts_rx *rx;
} sts_pred;

struct sts_query {
	TSQuery *query;
	uint32_t pattern_count;
	sts_pred **preds;	/* per pattern */
	int32_t *pred_counts;
	uint8_t *capture_styles;
	uint32_t capture_count;
	int32_t fold_capture, open_capture, close_capture;
};

static void sts_pred_wipe(sts_pred *a_pred)
{
	int32_t i;
	for (i = 0; i < a_pred->string_count; i++) {
		free(a_pred->strings[i]);
	}
	free(a_pred->strings);
	free(a_pred->lengths);
	rx_free(a_pred->rx);
}

static void sts_query_wipe(sts_query *a_query)
	/* Free everything `a_query' owns, and the query (not counted here). */
{
	uint32_t i;
	int32_t j;
	if (!a_query) {
		return;
	}
	if (a_query->preds) {
		for (i = 0; i < a_query->pattern_count; i++) {
			for (j = 0; j < a_query->pred_counts[i]; j++) {
				sts_pred_wipe(&a_query->preds[i][j]);
			}
			free(a_query->preds[i]);
		}
	}
	free(a_query->preds);
	free(a_query->pred_counts);
	free(a_query->capture_styles);
	if (a_query->query) {
		ts_query_delete(a_query->query);
	}
	free(a_query);
}

static void sts_message(char *a_message, int32_t a_capacity, const char *a_text, const char *a_detail)
{
	if (a_message && a_capacity > 0) {
		size_t n = strlen(a_text), m = a_detail ? strlen(a_detail) : 0;
		size_t l_room = (size_t)a_capacity - 1;
		if (n > l_room) {
			n = l_room;
		}
		memcpy(a_message, a_text, n);
		if (m > l_room - n) {
			m = l_room - n;
		}
		if (m) {
			memcpy(a_message + n, a_detail, m);
		}
		a_message[n + m] = 0;
	}
}

static int sts_query_read_predicates(sts_query *a_query, uint32_t a_pattern, char *a_message, int32_t a_capacity)
	/* Turn pattern `a_pattern''s predicate steps into checks. 0 for one we do not support. */
{
	uint32_t l_step_count = 0, i = 0;
	const TSQueryPredicateStep *l_steps = ts_query_predicates_for_pattern(a_query->query, a_pattern, &l_step_count);
	sts_pred *l_preds = NULL;
	int32_t l_count = 0;
	int l_ok = 1;

	while (l_ok && i < l_step_count) {
		uint32_t l_end = i, l_length = 0, l_argc;
		const char *l_name;
		sts_pred l_pred;
		int l_keep = 1;
		while (l_end < l_step_count && l_steps[l_end].type != TSQueryPredicateStepTypeDone) {
			l_end++;
		}
		l_argc = l_end - i - 1;
		memset(&l_pred, 0, sizeof(l_pred));
		l_pred.other_capture = -1;
		if (l_end == i || l_steps[i].type != TSQueryPredicateStepTypeString) {
			sts_message(a_message, a_capacity, "A predicate has no name.", NULL);
			l_ok = 0;
			break;
		}
		l_name = ts_query_string_value_for_id(a_query->query, l_steps[i].value_id, &l_length);
		if (l_length > 0 && l_name[l_length - 1] == '!') {
			l_keep = 0;	/* a directive (#set! and the like): nothing to check */
		} else {
			if (strncmp(l_name, "not-", 4) == 0) {
				l_pred.negate = 1;
				l_name += 4;
			}
			if (strcmp(l_name, "eq?") == 0) {
				l_pred.kind = STS_PRED_EQ;
			} else if (strcmp(l_name, "any-of?") == 0) {
				l_pred.kind = STS_PRED_ANY_OF;
			} else if (strcmp(l_name, "match?") == 0) {
				l_pred.kind = STS_PRED_MATCH;
			} else {
				sts_message(a_message, a_capacity, "Unsupported predicate: #", ts_query_string_value_for_id(a_query->query, l_steps[i].value_id, &l_length));
				l_ok = 0;
			}
			if (l_ok && (l_argc < 2 || l_steps[i + 1].type != TSQueryPredicateStepTypeCapture
					|| (l_pred.kind != STS_PRED_ANY_OF && l_argc != 2))) {
				sts_message(a_message, a_capacity, "Wrong arguments for predicate: #", ts_query_string_value_for_id(a_query->query, l_steps[i].value_id, &l_length));
				l_ok = 0;
			}
			if (l_ok) {
				uint32_t j;
				l_pred.capture = l_steps[i + 1].value_id;
				if (l_pred.kind == STS_PRED_EQ && l_steps[i + 2].type == TSQueryPredicateStepTypeCapture) {
					l_pred.other_capture = (int32_t)l_steps[i + 2].value_id;
				} else {
					l_pred.strings = (uint32_t **)calloc(l_argc, sizeof(uint32_t *));
					l_pred.lengths = (int32_t *)calloc(l_argc, sizeof(int32_t));
					for (j = i + 2; l_ok && j < l_end; j++) {
						const char *l_text;
						if (l_steps[j].type != TSQueryPredicateStepTypeString || !l_pred.strings || !l_pred.lengths) {
							sts_message(a_message, a_capacity, "Wrong arguments for predicate: #", ts_query_string_value_for_id(a_query->query, l_steps[i].value_id, &l_length));
							l_ok = 0;
							break;
						}
						l_text = ts_query_string_value_for_id(a_query->query, l_steps[j].value_id, &l_length);
						l_pred.strings[l_pred.string_count] = sts_utf8_to_32(l_text, l_length, &l_pred.lengths[l_pred.string_count]);
						l_pred.string_count++;
					}
					if (l_ok && l_pred.kind == STS_PRED_MATCH) {
						l_pred.rx = l_pred.strings[0] ? rx_compile(l_pred.strings[0], l_pred.lengths[0]) : NULL;
						if (!l_pred.rx) {
							sts_message(a_message, a_capacity, "Unsupported #match? pattern: ", ts_query_string_value_for_id(a_query->query, l_steps[i + 2].value_id, &l_length));
							l_ok = 0;
						}
					}
				}
			}
			if (!l_ok) {
				sts_pred_wipe(&l_pred);
			}
		}
		if (l_ok && l_keep) {
			sts_pred *l_more = (sts_pred *)realloc(l_preds, (size_t)(l_count + 1) * sizeof(sts_pred));
			if (l_more) {
				l_preds = l_more;
				l_preds[l_count++] = l_pred;
			} else {
				sts_pred_wipe(&l_pred);
			}
		}
		i = l_end + 1;
	}
	a_query->preds[a_pattern] = l_preds;
	a_query->pred_counts[a_pattern] = l_count;
	return l_ok;
}

static sts_query *sts_query_make(const TSLanguage *a_language, const char *a_source, uint32_t a_length,
	int32_t *an_error_offset, int32_t *an_error_kind, char *a_message, int32_t a_capacity)
	/* An uncounted query, or NULL. */
{
	static const char *l_kinds[] = {
		"", "Syntax error in the query.", "Unknown node type in the query.", "Unknown field name in the query.",
		"Unknown capture name in the query.", "A pattern that can never match.", "The query does not fit this language."
	};
	uint32_t l_offset = 0, i;
	TSQueryError l_error = TSQueryErrorNone;
	sts_query *l_query = (sts_query *)calloc(1, sizeof(sts_query));
	int32_t l_kind = STS_QUERY_OK;

	sts_message(a_message, a_capacity, "", NULL);
	if (!l_query) {
		return NULL;
	}
	l_query->query = ts_query_new(a_language, a_source, a_length, &l_offset, &l_error);
	if (!l_query->query) {
		l_kind = (int32_t)l_error;
		if (l_kind < 1 || l_kind > 6) {
			l_kind = STS_QUERY_SYNTAX;
		}
		sts_message(a_message, a_capacity, l_kinds[l_kind], NULL);
	} else {
		l_query->pattern_count = ts_query_pattern_count(l_query->query);
		l_query->capture_count = ts_query_capture_count(l_query->query);
		l_query->preds = (sts_pred **)calloc(l_query->pattern_count ? l_query->pattern_count : 1, sizeof(sts_pred *));
		l_query->pred_counts = (int32_t *)calloc(l_query->pattern_count ? l_query->pattern_count : 1, sizeof(int32_t));
		l_query->capture_styles = (uint8_t *)calloc(l_query->capture_count ? l_query->capture_count : 1, 1);
		l_query->fold_capture = l_query->open_capture = l_query->close_capture = -1;
		if (!l_query->preds || !l_query->pred_counts || !l_query->capture_styles) {
			l_kind = STS_QUERY_SYNTAX;
			sts_message(a_message, a_capacity, "Out of memory.", NULL);
		}
		for (i = 0; l_kind == STS_QUERY_OK && i < l_query->capture_count; i++) {
			uint32_t l_length = 0;
			const char *l_name = ts_query_capture_name_for_id(l_query->query, i, &l_length);
			l_query->capture_styles[i] = (uint8_t)sts_style_for_capture(l_name);
			if (strcmp(l_name, "fold") == 0) {
				l_query->fold_capture = (int32_t)i;
			} else if (strcmp(l_name, "open") == 0) {
				l_query->open_capture = (int32_t)i;
			} else if (strcmp(l_name, "close") == 0) {
				l_query->close_capture = (int32_t)i;
			}
		}
		for (i = 0; l_kind == STS_QUERY_OK && i < l_query->pattern_count; i++) {
			if (!sts_query_read_predicates(l_query, i, a_message, a_capacity)) {
				l_kind = STS_QUERY_PREDICATE;
				l_offset = ts_query_start_byte_for_pattern(l_query->query, i);
			}
		}
	}
	if (an_error_offset) {
		*an_error_offset = (int32_t)l_offset;
	}
	if (an_error_kind) {
		*an_error_kind = l_kind;
	}
	if (l_kind != STS_QUERY_OK) {
		sts_query_wipe(l_query);
		l_query = NULL;
	}
	return l_query;
}

sts_query *sts_query_new(const char *a_language, const char *a_source, int32_t a_length,
	int32_t *an_error_offset, int32_t *an_error_kind, char *a_message, int32_t a_capacity)
{
	const sts_lang *l_lang = sts_lang_named(a_language);
	sts_query *l_query;
	if (!l_lang || !a_source || a_length < 0) {
		if (an_error_offset) {
			*an_error_offset = 0;
		}
		if (an_error_kind) {
			*an_error_kind = STS_QUERY_LANGUAGE;
		}
		sts_message(a_message, a_capacity, "Unknown language.", NULL);
		return NULL;
	}
	l_query = sts_query_make(l_lang->language(), a_source, (uint32_t)a_length, an_error_offset, an_error_kind, a_message, a_capacity);
	if (l_query) {
		STS_LIVE_ADD(1);
	}
	return l_query;
}

void sts_query_free(sts_query *a_query)
{
	if (a_query) {
		STS_LIVE_ADD(-1);
		sts_query_wipe(a_query);
	}
}

int32_t sts_query_capture_count(const sts_query *a_query)
{
	return a_query ? (int32_t)a_query->capture_count : 0;
}

const char *sts_query_capture_name(const sts_query *a_query, int32_t a_index)
{
	uint32_t l_length = 0;
	if (!a_query || a_index < 0 || (uint32_t)a_index >= a_query->capture_count) {
		return NULL;
	}
	return ts_query_capture_name_for_id(a_query->query, (uint32_t)a_index, &l_length);
}

int32_t sts_query_pattern_count(const sts_query *a_query)
{
	return a_query ? (int32_t)a_query->pattern_count : 0;
}

/* ------------------------------------------------------------------------- */
/* Documents                                                                  */
/* ------------------------------------------------------------------------- */

struct sts_doc {
	const sts_lang *lang;
	TSParser *parser;
	TSTree *tree;
	uint32_t *text;
	int32_t count, capacity;
	int32_t *line_starts;
	int32_t line_count, line_capacity;
	sts_ints changed;	/* owed lines (pairs) that sts_doc_changed_lines has not answered yet */
	sts_ints reported;	/* owed lines it has answered, kept until sts_doc_changes_taken */
	int limited;	/* a limit cut the last result short */
	sts_query *highlights, *folds, *brackets;
	TSQueryCursor *cursor;
};

static const char *sts_read(void *a_payload, uint32_t a_byte, TSPoint a_point, uint32_t *a_bytes_read)
{
	const sts_doc *l_doc = (const sts_doc *)a_payload;
	uint32_t l_total = (uint32_t)l_doc->count * 4;
	(void)a_point;
	if (a_byte >= l_total) {
		*a_bytes_read = 0;
		return "";
	}
	*a_bytes_read = l_total - a_byte;
	return (const char *)l_doc->text + a_byte;
}

static uint32_t sts_decode(const uint8_t *a_bytes, uint32_t a_length, int32_t *a_code_point)
	/* One UTF-32 unit in the machine's byte order: always four bytes. */
{
	uint32_t l_unit;
	if (a_length < 4) {
		*a_code_point = -1;
		return a_length ? a_length : 1;
	}
	memcpy(&l_unit, a_bytes, 4);
	if (l_unit > 0x10FFFF || (l_unit >= 0xD800 && l_unit <= 0xDFFF)) {
		l_unit = 0xFFFD;
	}
	*a_code_point = (int32_t)l_unit;
	return 4;
}

static void sts_doc_parse(sts_doc *a_doc, TSTree *an_old)
	/* Parse the text into `tree', reusing `an_old' (already edited), and free `an_old'. */
{
	TSInput l_input;
	l_input.payload = a_doc;
	l_input.read = sts_read;
	l_input.encoding = TSInputEncodingCustom;
	l_input.decode = sts_decode;
	a_doc->tree = ts_parser_parse(a_doc->parser, an_old, l_input);
	if (an_old && a_doc->tree) {
		uint32_t l_count = 0, i;
		TSRange *l_ranges = ts_tree_get_changed_ranges(an_old, a_doc->tree, &l_count);
		for (i = 0; i < l_count; i++) {
			int32_t l_first = (int32_t)l_ranges[i].start_point.row;
			int32_t l_last = (int32_t)l_ranges[i].end_point.row;
			if (l_ranges[i].end_point.column == 0 && l_last > l_first) {
				l_last--;
			}
			sts_ints_put(&a_doc->changed, l_first);
			sts_ints_put(&a_doc->changed, l_last);
		}
		free(l_ranges);
	}
	if (an_old) {
		ts_tree_delete(an_old);
	}
}

static void sts_doc_index_lines(sts_doc *a_doc)
{
	int32_t i;
	a_doc->line_count = 0;
	for (i = -1; i < a_doc->count; i++) {
		if (i < 0 || a_doc->text[i] == '\n') {
			if (a_doc->line_count == a_doc->line_capacity) {
				int32_t l_capacity = a_doc->line_capacity ? a_doc->line_capacity * 2 : 256;
				int32_t *l_starts = (int32_t *)realloc(a_doc->line_starts, (size_t)l_capacity * sizeof(int32_t));
				if (!l_starts) {
					return;
				}
				a_doc->line_starts = l_starts;
				a_doc->line_capacity = l_capacity;
			}
			a_doc->line_starts[a_doc->line_count++] = i + 1;
		}
	}
}

static int32_t sts_doc_line_of(const sts_doc *a_doc, int32_t a_position)
{
	int32_t l_low = 0, l_high = a_doc->line_count - 1;
	while (l_low < l_high) {
		int32_t l_mid = (l_low + l_high + 1) / 2;
		if (a_doc->line_starts[l_mid] <= a_position) {
			l_low = l_mid;
		} else {
			l_high = l_mid - 1;
		}
	}
	return l_low;
}

static TSPoint sts_doc_point(const sts_doc *a_doc, int32_t a_position)
{
	TSPoint l_point;
	int32_t l_line = sts_doc_line_of(a_doc, a_position);
	l_point.row = (uint32_t)l_line;
	l_point.column = (uint32_t)(a_position - a_doc->line_starts[l_line]) * 4;
	return l_point;
}

static int32_t sts_doc_line_end(const sts_doc *a_doc, int32_t a_line)
	/* Where line `a_line''s text ends, before its line end. */
{
	int32_t l_end = (a_line + 1 < a_doc->line_count) ? a_doc->line_starts[a_line + 1] - 1 : a_doc->count;
	if (l_end > a_doc->line_starts[a_line] && a_doc->text[l_end - 1] == '\r') {
		l_end--;
	}
	return l_end;
}

static int sts_compare_pairs(const void *a, const void *b)
{
	const int32_t *x = (const int32_t *)a, *y = (const int32_t *)b;
	if (x[0] != y[0]) {
		return x[0] < y[0] ? -1 : 1;
	}
	return (x[1] > y[1]) - (x[1] < y[1]);
}

static void sts_doc_tidy(const sts_doc *a_doc, sts_ints *a_list)
	/* Sort the line ranges of `a_list', keep them in the text, and join those that touch. */
{
	int32_t *p = a_list->items;
	int32_t n = a_list->count / 2, i, l_out = 0;
	int32_t l_last_line = a_doc->line_count - 1;
	for (i = 0; i < n; i++) {
		if (p[2 * i] < 0) {
			p[2 * i] = 0;
		}
		if (p[2 * i] > l_last_line) {
			p[2 * i] = l_last_line;
		}
		if (p[2 * i + 1] > l_last_line) {
			p[2 * i + 1] = l_last_line;
		}
		if (p[2 * i + 1] < p[2 * i]) {
			p[2 * i + 1] = p[2 * i];
		}
	}
	if (n > 1) {
		qsort(p, (size_t)n, 2 * sizeof(int32_t), sts_compare_pairs);
	}
	for (i = 0; i < n; i++) {
		if (l_out > 0 && p[2 * i] <= p[2 * l_out - 1] + 1) {
			if (p[2 * i + 1] > p[2 * l_out - 1]) {
				p[2 * l_out - 1] = p[2 * i + 1];
			}
		} else {
			p[2 * l_out] = p[2 * i];
			p[2 * l_out + 1] = p[2 * i + 1];
			l_out++;
		}
	}
	a_list->count = 2 * l_out;
}

static void sts_move_ranges(sts_ints *a_list, int32_t a_first, int32_t an_old_last, int32_t a_delta)
	/* The lines `a_first' to `an_old_last' were edited and the lines after them moved by
	   `a_delta': move the ranges of `a_list' with the text. A range the edit touches
	   grows to hold the edit's lines. */
{
	int32_t i;
	for (i = 0; i + 1 < a_list->count; i += 2) {
		int32_t *p = a_list->items + i;
		if (p[1] < a_first) {
			/* before the edit */
		} else if (p[0] > an_old_last) {
			p[0] += a_delta;
			p[1] += a_delta;
		} else {
			if (p[0] > a_first) {
				p[0] = a_first;
			}
			if (p[1] < an_old_last) {
				p[1] = an_old_last;
			}
			p[1] += a_delta;
		}
	}
}

static sts_query *sts_doc_bundled(const sts_lang *a_lang, int a_kind)
{
	const char *l_source = sts_bundled_query(a_lang->name, a_kind);
	if (!l_source || !*l_source) {
		return NULL;
	}
	return sts_query_make(a_lang->language(), l_source, (uint32_t)strlen(l_source), NULL, NULL, NULL, 0);
}

sts_doc *sts_doc_new(const char *a_language)
{
	const sts_lang *l_lang = sts_lang_named(a_language);
	sts_doc *l_doc;
	if (!l_lang) {
		return NULL;
	}
	l_doc = (sts_doc *)sts_alloc(sizeof(sts_doc));
	if (!l_doc) {
		return NULL;
	}
	l_doc->lang = l_lang;
	l_doc->parser = ts_parser_new();
	l_doc->cursor = ts_query_cursor_new();
	if (!l_doc->parser || !l_doc->cursor || !ts_parser_set_language(l_doc->parser, l_lang->language())) {
		sts_doc_free(l_doc);
		return NULL;
	}
	ts_query_cursor_set_match_limit(l_doc->cursor, 2048);
	l_doc->highlights = sts_doc_bundled(l_lang, 0);
	l_doc->folds = sts_doc_bundled(l_lang, 1);
	l_doc->brackets = sts_doc_bundled(l_lang, 2);
	sts_doc_set_text(l_doc, NULL, 0);
	return l_doc;
}

void sts_doc_free(sts_doc *a_doc)
{
	if (a_doc) {
		if (a_doc->tree) {
			ts_tree_delete(a_doc->tree);
		}
		if (a_doc->parser) {
			ts_parser_delete(a_doc->parser);
		}
		if (a_doc->cursor) {
			ts_query_cursor_delete(a_doc->cursor);
		}
		sts_query_wipe(a_doc->highlights);
		sts_query_wipe(a_doc->folds);
		sts_query_wipe(a_doc->brackets);
		free(a_doc->text);
		free(a_doc->line_starts);
		free(a_doc->changed.items);
		free(a_doc->reported.items);
		sts_free(a_doc);
	}
}

static int sts_doc_reserve(sts_doc *a_doc, int32_t a_count)
{
	if (a_count > a_doc->capacity) {
		int32_t l_capacity = a_count + a_count / 2 + 64;
		uint32_t *l_text = (uint32_t *)realloc(a_doc->text, (size_t)l_capacity * sizeof(uint32_t));
		if (!l_text) {
			return 0;
		}
		a_doc->text = l_text;
		a_doc->capacity = l_capacity;
	}
	return 1;
}

void sts_doc_set_text(sts_doc *a_doc, const uint32_t *a_text, int32_t a_count)
{
	if (!a_doc || a_count < 0 || (a_count > 0 && !a_text) || !sts_doc_reserve(a_doc, a_count)) {
		return;
	}
	if (a_count > 0) {
		memcpy(a_doc->text, a_text, (size_t)a_count * sizeof(uint32_t));
	}
	a_doc->count = a_count;
	sts_doc_index_lines(a_doc);
	if (a_doc->tree) {
		ts_tree_delete(a_doc->tree);
		a_doc->tree = NULL;
	}
	ts_parser_reset(a_doc->parser);
	sts_doc_parse(a_doc, NULL);
	a_doc->changed.count = 0;
	a_doc->reported.count = 0;
	sts_ints_put(&a_doc->changed, 0);
	sts_ints_put(&a_doc->changed, a_doc->line_count - 1);
}

int32_t sts_doc_edit(sts_doc *a_doc, int32_t a_start, int32_t a_removed, const uint32_t *a_text, int32_t an_inserted)
{
	TSInputEdit l_edit;
	int32_t l_old_end_line, l_delta, l_tail;
	TSTree *l_old;

	if (!a_doc || a_start < 0 || a_removed < 0 || an_inserted < 0 || a_start > a_doc->count
			|| a_removed > a_doc->count - a_start || (an_inserted > 0 && !a_text)
			|| an_inserted > INT32_MAX / 8 - a_doc->count) {
		return 0;
	}
	if (!sts_doc_reserve(a_doc, a_doc->count - a_removed + an_inserted)) {
		return 0;
	}
	l_edit.start_byte = (uint32_t)a_start * 4;
	l_edit.old_end_byte = (uint32_t)(a_start + a_removed) * 4;
	l_edit.new_end_byte = (uint32_t)(a_start + an_inserted) * 4;
	l_edit.start_point = sts_doc_point(a_doc, a_start);
	l_edit.old_end_point = sts_doc_point(a_doc, a_start + a_removed);
	l_old_end_line = (int32_t)l_edit.old_end_point.row;

	l_tail = a_doc->count - a_start - a_removed;
	if (l_tail > 0 && a_removed != an_inserted) {
		memmove(a_doc->text + a_start + an_inserted, a_doc->text + a_start + a_removed, (size_t)l_tail * sizeof(uint32_t));
	}
	if (an_inserted > 0) {
		memcpy(a_doc->text + a_start, a_text, (size_t)an_inserted * sizeof(uint32_t));
	}
	a_doc->count += an_inserted - a_removed;
	sts_doc_index_lines(a_doc);
	l_edit.new_end_point = sts_doc_point(a_doc, a_start + an_inserted);

	/* Lines owed from earlier edits move with the text; then this edit's own lines. */
	l_delta = (int32_t)l_edit.new_end_point.row - l_old_end_line;
	sts_move_ranges(&a_doc->changed, (int32_t)l_edit.start_point.row, l_old_end_line, l_delta);
	sts_move_ranges(&a_doc->reported, (int32_t)l_edit.start_point.row, l_old_end_line, l_delta);
	sts_ints_put(&a_doc->changed, (int32_t)l_edit.start_point.row);
	sts_ints_put(&a_doc->changed, (int32_t)l_edit.new_end_point.row);

	l_old = a_doc->tree;
	a_doc->tree = NULL;
	if (l_old) {
		ts_tree_edit(l_old, &l_edit);
	}
	sts_doc_parse(a_doc, l_old);
	sts_doc_tidy(a_doc, &a_doc->changed);
	sts_doc_tidy(a_doc, &a_doc->reported);
	return 1;
}

uint32_t *sts_text_new(const uint32_t *a_text, int32_t a_count)
{
	uint32_t *l_block = NULL;
	if (a_text && a_count > 0) {
		l_block = (uint32_t *)sts_alloc((size_t)a_count * sizeof(uint32_t));
		if (l_block) {
			memcpy(l_block, a_text, (size_t)a_count * sizeof(uint32_t));
		}
	}
	return l_block;
}

int32_t sts_doc_count(const sts_doc *a_doc)
{
	return a_doc ? a_doc->count : 0;
}

int32_t sts_doc_line_count(const sts_doc *a_doc)
{
	return a_doc ? a_doc->line_count : 0;
}

int32_t sts_doc_line_start(const sts_doc *a_doc, int32_t a_line)
{
	return (a_doc && a_line >= 0 && a_line < a_doc->line_count) ? a_doc->line_starts[a_line] : 0;
}

int32_t sts_doc_has_error(const sts_doc *a_doc)
{
	return (a_doc && a_doc->tree) ? ts_node_has_error(ts_tree_root_node(a_doc->tree)) : 0;
}

const uint32_t *sts_doc_text(const sts_doc *a_doc)
{
	return a_doc ? a_doc->text : NULL;
}

/* ------------------------------------------------------------------------- */
/* Running queries                                                            */
/* ------------------------------------------------------------------------- */

static int sts_same_text(const sts_doc *a_doc, TSNode a_node, const uint32_t *a_text, int32_t a_length)
{
	uint32_t l_start = ts_node_start_byte(a_node) / 4, l_end = ts_node_end_byte(a_node) / 4;
	return (int32_t)(l_end - l_start) == a_length
		&& (a_length == 0 || memcmp(a_doc->text + l_start, a_text, (size_t)a_length * sizeof(uint32_t)) == 0);
}

static int sts_pred_holds(sts_doc *a_doc, const sts_pred *a_pred, TSNode a_node, const TSQueryMatch *a_match)
{
	uint32_t l_start = ts_node_start_byte(a_node) / 4, l_end = ts_node_end_byte(a_node) / 4;
	int l_result = 0;
	int32_t i;
	uint16_t j;
	if (l_end > (uint32_t)a_doc->count) {
		return 1;
	}
	switch (a_pred->kind) {
	case STS_PRED_EQ:
		if (a_pred->other_capture >= 0) {
			l_result = 1;
			for (j = 0; j < a_match->capture_count; j++) {
				if ((int32_t)a_match->captures[j].index == a_pred->other_capture) {
					TSNode l_other = a_match->captures[j].node;
					uint32_t l_from = ts_node_start_byte(l_other) / 4;
					l_result = l_result && ts_node_end_byte(l_other) / 4 <= (uint32_t)a_doc->count
						&& sts_same_text(a_doc, a_node, a_doc->text + l_from, (int32_t)(ts_node_end_byte(l_other) / 4 - l_from));
				}
			}
		} else {
			l_result = sts_same_text(a_doc, a_node, a_pred->strings[0], a_pred->lengths[0]);
		}
		break;
	case STS_PRED_ANY_OF:
		for (i = 0; i < a_pred->string_count && !l_result; i++) {
			l_result = sts_same_text(a_doc, a_node, a_pred->strings[i], a_pred->lengths[i]);
		}
		break;
	default:
		l_result = rx_search(a_pred->rx, a_doc->text + l_start, (int)(l_end - l_start), &a_doc->limited);
		break;
	}
	return a_pred->negate ? !l_result : l_result;
}

static int sts_match_holds(sts_doc *a_doc, const sts_query *a_query, const TSQueryMatch *a_match)
	/* Do the pattern's text predicates hold for the captures `a_match' has? */
{
	int32_t i;
	uint16_t j;
	for (i = 0; i < a_query->pred_counts[a_match->pattern_index]; i++) {
		const sts_pred *l_pred = &a_query->preds[a_match->pattern_index][i];
		for (j = 0; j < a_match->capture_count; j++) {
			if (a_match->captures[j].index == l_pred->capture
					&& !sts_pred_holds(a_doc, l_pred, a_match->captures[j].node, a_match)) {
				return 0;
			}
		}
	}
	return 1;
}

static int sts_next_capture(sts_doc *a_doc, TSQueryMatch *a_match, uint32_t *an_index)
	/* The cursor's next capture, noting on the document when the match limit was passed. */
{
	int l_result = ts_query_cursor_next_capture(a_doc->cursor, a_match, an_index);
	if (ts_query_cursor_did_exceed_match_limit(a_doc->cursor)) {
		a_doc->limited = 1;
	}
	return l_result;
}

static int sts_next_match(sts_doc *a_doc, TSQueryMatch *a_match)
{
	int l_result = ts_query_cursor_next_match(a_doc->cursor, a_match);
	if (ts_query_cursor_did_exceed_match_limit(a_doc->cursor)) {
		a_doc->limited = 1;
	}
	return l_result;
}

typedef struct {
	int32_t start, end, pattern, value, order;
} sts_hit;

static int sts_compare_hits(const void *a, const void *b)
	/* Outer before inner, then earlier pattern first, so the last painted wins. */
{
	const sts_hit *x = (const sts_hit *)a, *y = (const sts_hit *)b;
	if (x->start != y->start) {
		return x->start < y->start ? -1 : 1;
	}
	if (x->end != y->end) {
		return x->end > y->end ? -1 : 1;
	}
	if (x->pattern != y->pattern) {
		return x->pattern < y->pattern ? -1 : 1;
	}
	return (x->order > y->order) - (x->order < y->order);
}

int32_t *sts_doc_spans(sts_doc *a_doc, int32_t a_first_line, int32_t a_last_line, int32_t *a_count)
{
	sts_ints l_spans = { NULL, 0, 0 };
	sts_hit *l_hits = NULL;
	int32_t l_hit_count = 0, l_hit_capacity = 0, l_from, l_to, l_line, i;
	uint8_t *l_paint;
	TSQueryMatch l_match;
	uint32_t l_index;
	const sts_query *l_query;

	if (a_count) {
		*a_count = 0;
	}
	if (!a_doc || !a_doc->tree || !a_doc->highlights) {
		return NULL;
	}
	a_doc->limited = 0;
	if (a_first_line < 0) {
		a_first_line = 0;
	}
	if (a_last_line > a_doc->line_count - 1) {
		a_last_line = a_doc->line_count - 1;
	}
	if (a_last_line < a_first_line) {
		return NULL;
	}
	l_query = a_doc->highlights;
	l_from = a_doc->line_starts[a_first_line];
	l_to = sts_doc_line_end(a_doc, a_last_line);
	if (l_to <= l_from) {
		return NULL;
	}
	l_paint = (uint8_t *)calloc((size_t)(l_to - l_from), 1);
	if (!l_paint) {
		return NULL;
	}
	ts_query_cursor_set_byte_range(a_doc->cursor, (uint32_t)l_from * 4, (uint32_t)l_to * 4);
	ts_query_cursor_exec(a_doc->cursor, l_query->query, ts_tree_root_node(a_doc->tree));
	while (sts_next_capture(a_doc, &l_match, &l_index)) {
		const TSQueryCapture *l_capture = &l_match.captures[l_index];
		int32_t l_style = l_query->capture_styles[l_capture->index];
		if (!sts_match_holds(a_doc, l_query, &l_match)) {
			ts_query_cursor_remove_match(a_doc->cursor, l_match.id);
		} else if (l_style != STS_STYLE_PLAIN) {
			int32_t l_start = (int32_t)(ts_node_start_byte(l_capture->node) / 4);
			int32_t l_end = (int32_t)(ts_node_end_byte(l_capture->node) / 4);
			if (l_start < l_from) {
				l_start = l_from;
			}
			if (l_end > l_to) {
				l_end = l_to;
			}
			if (l_end > l_start) {
				if (l_hit_count == l_hit_capacity) {
					int32_t l_capacity = l_hit_capacity ? l_hit_capacity * 2 : 256;
					sts_hit *l_more = (sts_hit *)realloc(l_hits, (size_t)l_capacity * sizeof(sts_hit));
					if (!l_more) {
						break;
					}
					l_hits = l_more;
					l_hit_capacity = l_capacity;
				}
				l_hits[l_hit_count].start = l_start;
				l_hits[l_hit_count].end = l_end;
				l_hits[l_hit_count].pattern = l_match.pattern_index;
				l_hits[l_hit_count].value = l_style;
				l_hits[l_hit_count].order = l_hit_count;
				l_hit_count++;
			}
		}
	}
	if (l_hit_count > 1) {
		qsort(l_hits, (size_t)l_hit_count, sizeof(sts_hit), sts_compare_hits);
	}
	for (i = 0; i < l_hit_count; i++) {
		memset(l_paint + (l_hits[i].start - l_from), l_hits[i].value, (size_t)(l_hits[i].end - l_hits[i].start));
	}
	free(l_hits);
	for (l_line = a_first_line; l_line <= a_last_line; l_line++) {
		int32_t l_begin = a_doc->line_starts[l_line], l_end = sts_doc_line_end(a_doc, l_line);
		int32_t l_at = l_begin;
		while (l_at < l_end) {
			uint8_t l_style = l_paint[l_at - l_from];
			int32_t l_run = l_at + 1;
			while (l_run < l_end && l_paint[l_run - l_from] == l_style) {
				l_run++;
			}
			if (l_style != STS_STYLE_PLAIN) {
				sts_ints_put(&l_spans, l_line);
				sts_ints_put(&l_spans, l_at - l_begin);
				sts_ints_put(&l_spans, l_run - l_at);
				sts_ints_put(&l_spans, l_style);
			}
			l_at = l_run;
		}
	}
	free(l_paint);
	return sts_ints_take(&l_spans, 4, a_count);
}

int32_t *sts_doc_changed_lines(sts_doc *a_doc, int32_t *a_count)
{
	int32_t *l_block = NULL;
	int32_t i;
	if (a_count) {
		*a_count = 0;
	}
	if (!a_doc) {
		return NULL;
	}
	/* Everything owed is answered, and from now on counts as reported. */
	for (i = 0; i < a_doc->changed.count; i++) {
		sts_ints_put(&a_doc->reported, a_doc->changed.items[i]);
	}
	a_doc->changed.count = 0;
	sts_doc_tidy(a_doc, &a_doc->reported);
	if (a_doc->reported.count > 0) {
		l_block = (int32_t *)sts_alloc((size_t)a_doc->reported.count * sizeof(int32_t));
		if (l_block) {
			memcpy(l_block, a_doc->reported.items, (size_t)a_doc->reported.count * sizeof(int32_t));
			if (a_count) {
				*a_count = a_doc->reported.count / 2;
			}
		}
	}
	return l_block;
}

void sts_doc_changes_taken(sts_doc *a_doc)
{
	if (a_doc) {
		a_doc->reported.count = 0;
	}
}

int32_t sts_doc_was_limited(const sts_doc *a_doc)
{
	return a_doc && a_doc->limited;
}

int32_t *sts_doc_folds(sts_doc *a_doc, int32_t *a_count)
{
	sts_ints l_folds = { NULL, 0, 0 };
	TSQueryMatch l_match;
	uint32_t l_index;
	const sts_query *l_query;
	int32_t n, i, l_out = 0;

	if (a_count) {
		*a_count = 0;
	}
	if (!a_doc || !a_doc->tree || !a_doc->folds || a_doc->folds->fold_capture < 0) {
		return NULL;
	}
	a_doc->limited = 0;
	l_query = a_doc->folds;
	ts_query_cursor_set_byte_range(a_doc->cursor, 0, UINT32_MAX);
	ts_query_cursor_exec(a_doc->cursor, l_query->query, ts_tree_root_node(a_doc->tree));
	while (sts_next_capture(a_doc, &l_match, &l_index)) {
		const TSQueryCapture *l_capture = &l_match.captures[l_index];
		if (!sts_match_holds(a_doc, l_query, &l_match)) {
			ts_query_cursor_remove_match(a_doc->cursor, l_match.id);
		} else if ((int32_t)l_capture->index == l_query->fold_capture) {
			TSPoint l_start = ts_node_start_point(l_capture->node), l_end = ts_node_end_point(l_capture->node);
			int32_t l_last = (int32_t)l_end.row;
			if (l_end.column == 0 && l_last > (int32_t)l_start.row) {
				l_last--;
			}
			if (l_last > (int32_t)l_start.row) {
				sts_ints_put(&l_folds, (int32_t)l_start.row);
				sts_ints_put(&l_folds, l_last);
			}
		}
	}
	n = l_folds.count / 2;
	if (n > 1) {
		qsort(l_folds.items, (size_t)n, 2 * sizeof(int32_t), sts_compare_pairs);
	}
	for (i = 0; i < n; i++) {
		int32_t *p = l_folds.items;
		if (l_out == 0 || p[2 * i] != p[2 * l_out - 2] || p[2 * i + 1] != p[2 * l_out - 1]) {
			p[2 * l_out] = p[2 * i];
			p[2 * l_out + 1] = p[2 * i + 1];
			l_out++;
		}
	}
	l_folds.count = 2 * l_out;
	return sts_ints_take(&l_folds, 2, a_count);
}

/* How many ancestors of a bracket are tried: Eiffel's `once' is six below the routine
   whose `end' closes it. */
#define STS_BRACKET_LEVELS 8

static int sts_doc_pair_of(sts_doc *a_doc, TSNode a_leaf, int32_t *a_pair)
	/* The bracket pair `a_leaf' is one end of, looked for in the patterns that start at
	   one of its nearest ancestors: its parent first, then outward.
	   A start depth of 1 and not 0: tree-sitter starts a pattern with a wildcard root,
	   such as (_ "(" @open ")" @close), at the wildcard's first child. */
{
	const sts_query *l_query = a_doc->brackets;
	TSNode l_root = ts_node_parent(a_leaf);
	int l_depth, l_found = 0;
	ts_query_cursor_set_byte_range(a_doc->cursor, 0, UINT32_MAX);
	ts_query_cursor_set_max_start_depth(a_doc->cursor, 1);
	for (l_depth = 0; l_depth < STS_BRACKET_LEVELS && !l_found && !ts_node_is_null(l_root); l_depth++) {
		TSQueryMatch l_match;
		uint32_t l_best = UINT32_MAX;
		ts_query_cursor_exec(a_doc->cursor, l_query->query, l_root);
		while (sts_next_match(a_doc, &l_match)) {
			TSNode l_open = a_leaf, l_close = a_leaf;
			int l_has_open = 0, l_has_close = 0;
			uint16_t j;
			if (!sts_match_holds(a_doc, l_query, &l_match)) {
				continue;
			}
			for (j = 0; j < l_match.capture_count; j++) {
				if ((int32_t)l_match.captures[j].index == l_query->open_capture) {
					l_open = l_match.captures[j].node;
					l_has_open = 1;
				} else if ((int32_t)l_match.captures[j].index == l_query->close_capture) {
					l_close = l_match.captures[j].node;
					l_has_close = 1;
				}
			}
			if (l_has_open && l_has_close && ts_node_end_byte(l_open) <= ts_node_start_byte(l_close)
					&& (ts_node_eq(l_open, a_leaf) || ts_node_eq(l_close, a_leaf))) {
				/* Of several pairs with this end (two bracketed parts in one node), the nearest. */
				uint32_t l_gap = ts_node_start_byte(l_close) - ts_node_end_byte(l_open);
				if (!l_found || l_gap < l_best) {
					l_found = 1;
					l_best = l_gap;
					a_pair[0] = (int32_t)(ts_node_start_byte(l_open) / 4);
					a_pair[1] = (int32_t)(ts_node_end_byte(l_open) / 4) - a_pair[0];
					a_pair[2] = (int32_t)(ts_node_start_byte(l_close) / 4);
					a_pair[3] = (int32_t)(ts_node_end_byte(l_close) / 4) - a_pair[2];
				}
			}
		}
		l_root = ts_node_parent(l_root);
	}
	ts_query_cursor_set_max_start_depth(a_doc->cursor, UINT32_MAX);
	return l_found;
}

int32_t sts_doc_bracket_at(sts_doc *a_doc, int32_t a_position, int32_t *a_pair)
{
	int l_try;
	if (!a_doc || !a_pair || !a_doc->tree || !a_doc->brackets
			|| a_doc->brackets->open_capture < 0 || a_doc->brackets->close_capture < 0) {
		return 0;
	}
	a_doc->limited = 0;
	for (l_try = 0; l_try < 2; l_try++) {
		int32_t l_at = a_position - l_try;
		if (l_at >= 0 && l_at < a_doc->count) {
			TSNode l_leaf = ts_node_descendant_for_byte_range(ts_tree_root_node(a_doc->tree), (uint32_t)l_at * 4, (uint32_t)l_at * 4 + 4);
			if (!ts_node_is_null(l_leaf) && ts_node_child_count(l_leaf) == 0
					&& ts_node_start_byte(l_leaf) <= (uint32_t)l_at * 4 && ts_node_end_byte(l_leaf) > (uint32_t)l_at * 4
					&& sts_doc_pair_of(a_doc, l_leaf, a_pair)) {
				return 1;
			}
		}
	}
	return 0;
}

int32_t *sts_doc_run(sts_doc *a_doc, sts_query *a_query, int32_t a_first, int32_t a_last, int32_t *a_count)
{
	sts_ints l_out = { NULL, 0, 0 };
	TSQueryMatch l_match;
	uint32_t l_index;

	if (a_count) {
		*a_count = 0;
	}
	if (!a_doc || !a_query || !a_doc->tree) {
		return NULL;
	}
	a_doc->limited = 0;
	if (a_first < 0) {
		a_first = 0;
	}
	if (a_last > a_doc->count - 1) {
		a_last = a_doc->count - 1;
	}
	if (a_last < a_first) {
		return NULL;
	}
	ts_query_cursor_set_byte_range(a_doc->cursor, (uint32_t)a_first * 4, (uint32_t)(a_last + 1) * 4);
	ts_query_cursor_exec(a_doc->cursor, a_query->query, ts_tree_root_node(a_doc->tree));
	while (sts_next_capture(a_doc, &l_match, &l_index)) {
		const TSQueryCapture *l_capture = &l_match.captures[l_index];
		if (!sts_match_holds(a_doc, a_query, &l_match)) {
			ts_query_cursor_remove_match(a_doc->cursor, l_match.id);
		} else {
			int32_t l_start = (int32_t)(ts_node_start_byte(l_capture->node) / 4);
			sts_ints_put(&l_out, l_match.pattern_index);
			sts_ints_put(&l_out, (int32_t)l_capture->index);
			sts_ints_put(&l_out, l_start);
			sts_ints_put(&l_out, (int32_t)(ts_node_end_byte(l_capture->node) / 4) - l_start);
		}
	}
	return sts_ints_take(&l_out, 4, a_count);
}

/* ------------------------------------------------------------------------- */
/* Trees                                                                      */
/* ------------------------------------------------------------------------- */

char *sts_doc_sexp(sts_doc *a_doc)
{
	char *l_text, *l_copy;
	size_t n;
	if (!a_doc || !a_doc->tree) {
		return NULL;
	}
	l_text = ts_node_string(ts_tree_root_node(a_doc->tree));
	if (!l_text) {
		return NULL;
	}
	n = strlen(l_text);
	l_copy = (char *)sts_alloc(n + 1);
	if (l_copy) {
		memcpy(l_copy, l_text, n + 1);
	}
	free(l_text);
	return l_copy;
}

static const char *sts_node_info(TSNode a_node, int32_t *an_info)
{
	if (ts_node_is_null(a_node)) {
		return NULL;
	}
	if (an_info) {
		TSPoint l_end = ts_node_end_point(a_node);
		an_info[0] = (int32_t)(ts_node_start_byte(a_node) / 4);
		an_info[1] = (int32_t)(ts_node_end_byte(a_node) / 4) - an_info[0];
		an_info[2] = (int32_t)ts_node_start_point(a_node).row;
		an_info[3] = (int32_t)l_end.row;
		if (l_end.column == 0 && an_info[3] > an_info[2]) {
			an_info[3]--;
		}
		an_info[4] = ts_node_is_named(a_node);
		an_info[5] = ts_node_is_error(a_node);
		an_info[6] = ts_node_is_missing(a_node);
	}
	return ts_node_type(a_node);
}

const char *sts_doc_node_at(sts_doc *a_doc, int32_t a_position, int32_t *an_info)
{
	if (!a_doc || !a_doc->tree || a_position < 0 || a_position >= a_doc->count) {
		return NULL;
	}
	return sts_node_info(ts_node_descendant_for_byte_range(ts_tree_root_node(a_doc->tree),
		(uint32_t)a_position * 4, (uint32_t)a_position * 4 + 4), an_info);
}

struct sts_cursor {
	TSTreeCursor cursor;
};

sts_cursor *sts_cursor_new(sts_doc *a_doc)
{
	sts_cursor *l_cursor;
	if (!a_doc || !a_doc->tree) {
		return NULL;
	}
	l_cursor = (sts_cursor *)sts_alloc(sizeof(sts_cursor));
	if (l_cursor) {
		l_cursor->cursor = ts_tree_cursor_new(ts_tree_root_node(a_doc->tree));
	}
	return l_cursor;
}

void sts_cursor_free(sts_cursor *a_cursor)
{
	if (a_cursor) {
		ts_tree_cursor_delete(&a_cursor->cursor);
		sts_free(a_cursor);
	}
}

int32_t sts_cursor_goto_first_child(sts_cursor *a_cursor)
{
	return a_cursor ? ts_tree_cursor_goto_first_child(&a_cursor->cursor) : 0;
}

int32_t sts_cursor_goto_next_sibling(sts_cursor *a_cursor)
{
	return a_cursor ? ts_tree_cursor_goto_next_sibling(&a_cursor->cursor) : 0;
}

int32_t sts_cursor_goto_parent(sts_cursor *a_cursor)
{
	return a_cursor ? ts_tree_cursor_goto_parent(&a_cursor->cursor) : 0;
}

const char *sts_cursor_node(sts_cursor *a_cursor, int32_t *an_info)
{
	return a_cursor ? sts_node_info(ts_tree_cursor_current_node(&a_cursor->cursor), an_info) : NULL;
}
