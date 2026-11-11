/*
 * simple_tree_sitter: the C side of the Eiffel library.
 *
 * A document is a text, its syntax tree and its language's three editor queries
 * (highlights, folds, brackets). The text is kept as UTF-32 in the machine's byte
 * order, so every position below is a count of characters (Unicode code points)
 * from 0. Lines count from 0 too; the Eiffel classes add the 1.
 *
 * Results come back as blocks of int32_t that the caller frees with sts_free.
 * A result with no items is NULL with a count of 0.
 *
 * A document belongs to one thread at a time. Different documents may be used on
 * different threads at once.
 */

#ifndef SIMPLE_TREE_SITTER_H
#define SIMPLE_TREE_SITTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sts_doc sts_doc;
typedef struct sts_query sts_query;
typedef struct sts_cursor sts_cursor;

/* Styles a highlight capture resolves to; 1 to 6 are GUI_CODE_EDITOR's. */
enum {
	STS_STYLE_PLAIN = 0,
	STS_STYLE_KEYWORD = 1,
	STS_STYLE_TYPE = 2,
	STS_STYLE_STRING = 3,
	STS_STYLE_COMMENT = 4,
	STS_STYLE_NUMBER = 5,
	STS_STYLE_OPERATOR = 6,
	STS_STYLE_FUNCTION = 7,
	STS_STYLE_CONSTANT = 8,
	STS_STYLE_VARIABLE = 9,
	STS_STYLE_TAG = 10
};

/* Query errors: 1 to 6 are tree-sitter's own. */
enum {
	STS_QUERY_OK = 0,
	STS_QUERY_SYNTAX = 1,
	STS_QUERY_NODE_TYPE = 2,
	STS_QUERY_FIELD = 3,
	STS_QUERY_CAPTURE = 4,
	STS_QUERY_STRUCTURE = 5,
	STS_QUERY_LANGUAGE = 6,
	STS_QUERY_PREDICATE = 7	/* a predicate, or a #match? pattern, this library does not support */
};

/* Languages */

int32_t sts_language_count(void);
const char *sts_language_name(int32_t a_index);
	/* The name of language `a_index' (from 0); NULL when out of range. */
int32_t sts_language_has(const char *a_name);
const char *sts_language_for_extension(const char *an_extension);
	/* The language for a file extension without its dot ("e", "ecf"), compared without
	   regard to case; NULL when none. */
int32_t sts_style_for_capture(const char *a_name);
	/* The style a capture name gives ("keyword.control" gives STS_STYLE_KEYWORD). */

/* Documents */

sts_doc *sts_doc_new(const char *a_language);
	/* NULL for an unknown language. The text is empty. */
void sts_doc_free(sts_doc *a_doc);
void sts_doc_set_text(sts_doc *a_doc, const uint32_t *a_text, int32_t a_count);
	/* Replace the whole text and parse it from nothing. */
int32_t sts_doc_edit(sts_doc *a_doc, int32_t a_start, int32_t a_removed, const uint32_t *a_text, int32_t an_inserted);
	/* Put `an_inserted' characters of `a_text' in place of `a_removed' characters from
	   `a_start', and parse again reusing the old tree. 0 when the range is not in the text. */
int32_t sts_doc_count(const sts_doc *a_doc);
int32_t sts_doc_line_count(const sts_doc *a_doc);
int32_t sts_doc_line_start(const sts_doc *a_doc, int32_t a_line);
int32_t sts_doc_has_error(const sts_doc *a_doc);
const uint32_t *sts_doc_text(const sts_doc *a_doc);
	/* The document's own text; valid until the next change. */
uint32_t *sts_text_new(const uint32_t *a_text, int32_t a_count);
	/* A copy of `a_count' characters in a block of its own (free with sts_free), for
	   handing a text to the thread that owns a document. NULL when there are none. */

/* Results */

int32_t *sts_doc_spans(sts_doc *a_doc, int32_t a_first_line, int32_t a_last_line, int32_t *a_count);
	/* Four integers per span: line, column, length, style. In order, never overlapping,
	   never across a line end. */
int32_t *sts_doc_changed_lines(sts_doc *a_doc, int32_t *a_count);
	/* Two integers per range: the first and last line of every range the document owes,
	   in the text's line numbers as they are now, in order, joined where they touch.
	   A line changed by sts_doc_set_text or sts_doc_edit is owed until
	   sts_doc_changes_taken; answering it here forgets nothing. */
void sts_doc_changes_taken(sts_doc *a_doc);
	/* Forget the ranges the last sts_doc_changed_lines answered. Lines changed after
	   that call stay owed. */
int32_t *sts_doc_folds(sts_doc *a_doc, int32_t *a_count);
	/* Two integers per fold: its first and last line. */
int32_t sts_doc_bracket_at(sts_doc *a_doc, int32_t a_position, int32_t *a_pair);
	/* When the character at `a_position', or failing that the one before it, is one end
	   of a bracket pair: 1, with the open end's start and length and the close end's
	   start and length in `a_pair' [0..3]. Otherwise 0. */

int32_t sts_doc_was_limited(const sts_doc *a_doc);
	/* Did a limit cut short the last call of sts_doc_spans, sts_doc_folds,
	   sts_doc_bracket_at or sts_doc_run on this document? The limits are the query
	   cursor's matches in progress and the steps one #match? test may take. */

/* Trees, for tools and tests */

char *sts_doc_sexp(sts_doc *a_doc);
	/* The tree as an S-expression (free with sts_free); NULL without a tree. */
const char *sts_doc_node_at(sts_doc *a_doc, int32_t a_position, int32_t *an_info);
	/* The type of the smallest node over the character at `a_position', with its start,
	   length, first line, last line, and whether it is named, an error and missing in
	   `an_info' [0..6]. NULL without a tree. */
sts_cursor *sts_cursor_new(sts_doc *a_doc);
	/* A cursor on the root node; NULL without a tree. Valid until the document changes. */
void sts_cursor_free(sts_cursor *a_cursor);
int32_t sts_cursor_goto_first_child(sts_cursor *a_cursor);
int32_t sts_cursor_goto_next_sibling(sts_cursor *a_cursor);
int32_t sts_cursor_goto_parent(sts_cursor *a_cursor);
const char *sts_cursor_node(sts_cursor *a_cursor, int32_t *an_info);
	/* As sts_doc_node_at, for the cursor's node. */

/* Queries of the caller's own */

sts_query *sts_query_new(const char *a_language, const char *a_source, int32_t a_length,
	int32_t *an_error_offset, int32_t *an_error_kind, char *a_message, int32_t a_capacity);
	/* NULL on an error, with its byte offset in the source, its kind and a short message. */
void sts_query_free(sts_query *a_query);
int32_t sts_query_capture_count(const sts_query *a_query);
const char *sts_query_capture_name(const sts_query *a_query, int32_t a_index);
int32_t sts_query_pattern_count(const sts_query *a_query);
int32_t *sts_doc_run(sts_doc *a_doc, sts_query *a_query, int32_t a_first, int32_t a_last, int32_t *a_count);
	/* Four integers per capture: pattern, capture, start, length, for the captures that
	   touch the characters `a_first' to `a_last'. In order. */

/* Memory */

void sts_free(void *a_block);
int32_t sts_live_blocks(void);
	/* Blocks and handles given out and not yet freed, in the whole process. */

#ifdef __cplusplus
}
#endif

#endif
