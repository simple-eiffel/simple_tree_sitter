note
	description: "[
		The C wrapper's functions (Clib/simple_tree_sitter.h), one external each.
		Positions and lines count from 0 here; the classes that inherit this add the 1.

		Blocking calls. EiffelStudio's collector stops every thread before it
		collects, and a thread inside a C external is only stopped when the
		external returns: without more, a collection on an editor's processor
		waits for a parse on the worker's (30 ms measured for one call on a text
		of 40,000 lines, 128 ms on 160,000). An external marked `blocking' is left
		out of that wait. The ones a TS_WORKER calls are marked so. A blocking
		external must not be handed the address of an Eiffel object, which a
		collection may move while the C code runs: these take C blocks and the
		addresses of local integers only. `c_doc_set_text' and `c_doc_edit' take
		the characters of an Eiffel string and so are not blocking; the worker
		uses `c_doc_set_text_block' and `c_doc_edit_block'.
	]"

class
	TS_API

feature {NONE} -- Languages

	c_language_count: INTEGER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_INTEGER) sts_language_count();"
		end

	c_language_name (an_index: INTEGER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_language_name((int32_t) $an_index);"
		end

	c_language_has (a_name: POINTER): BOOLEAN
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_BOOLEAN) (sts_language_has((const char *) $a_name) != 0);"
		end

	c_language_for_extension (an_extension: POINTER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_language_for_extension((const char *) $an_extension);"
		end

	c_style_for_capture (a_name: POINTER): INTEGER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_INTEGER) sts_style_for_capture((const char *) $a_name);"
		end

feature {NONE} -- Documents

	c_doc_new (a_language: POINTER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_new((const char *) $a_language);"
		end

	c_doc_free (a_doc: POINTER)
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"sts_doc_free((sts_doc *) $a_doc);"
		end

	c_doc_set_text (a_doc, a_text: POINTER; a_count: INTEGER)
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"sts_doc_set_text((sts_doc *) $a_doc, (const uint32_t *) $a_text, (int32_t) $a_count);"
		end

	c_doc_edit (a_doc: POINTER; a_start, a_removed: INTEGER; a_text: POINTER; an_inserted: INTEGER): BOOLEAN
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_BOOLEAN) (sts_doc_edit((sts_doc *) $a_doc, (int32_t) $a_start, (int32_t) $a_removed, (const uint32_t *) $a_text, (int32_t) $an_inserted) != 0);"
		end

	c_doc_set_text_block (a_doc, a_text: POINTER; a_count: INTEGER)
			-- As `c_doc_set_text', for a text in a C block. See `Blocking calls' in the
			-- class's notes.
		external
			"C blocking inline use %"simple_tree_sitter.h%""
		alias
			"sts_doc_set_text((sts_doc *) $a_doc, (const uint32_t *) $a_text, (int32_t) $a_count);"
		end

	c_doc_edit_block (a_doc: POINTER; a_start, a_removed: INTEGER; a_text: POINTER; an_inserted: INTEGER): BOOLEAN
			-- As `c_doc_edit', for a text in a C block.
		external
			"C blocking inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_BOOLEAN) (sts_doc_edit((sts_doc *) $a_doc, (int32_t) $a_start, (int32_t) $a_removed, (const uint32_t *) $a_text, (int32_t) $an_inserted) != 0);"
		end

	c_doc_count (a_doc: POINTER): INTEGER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_INTEGER) sts_doc_count((const sts_doc *) $a_doc);"
		end

	c_doc_line_count (a_doc: POINTER): INTEGER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_INTEGER) sts_doc_line_count((const sts_doc *) $a_doc);"
		end

	c_doc_line_start (a_doc: POINTER; a_line: INTEGER): INTEGER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_INTEGER) sts_doc_line_start((const sts_doc *) $a_doc, (int32_t) $a_line);"
		end

	c_doc_has_error (a_doc: POINTER): BOOLEAN
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_BOOLEAN) (sts_doc_has_error((const sts_doc *) $a_doc) != 0);"
		end

	c_doc_text (a_doc: POINTER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_text((const sts_doc *) $a_doc);"
		end

	c_text_new (a_text: POINTER; a_count: INTEGER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_text_new((const uint32_t *) $a_text, (int32_t) $a_count);"
		end

feature {NONE} -- Results

	c_doc_spans (a_doc: POINTER; a_first_line, a_last_line: INTEGER; a_count: TYPED_POINTER [INTEGER]): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_spans((sts_doc *) $a_doc, (int32_t) $a_first_line, (int32_t) $a_last_line, (int32_t *) $a_count);"
		end

	c_doc_spans_of (a_doc, a_ranges: POINTER; a_range_count: INTEGER; a_count: TYPED_POINTER [INTEGER]): POINTER
		external
			"C blocking inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_spans_of((sts_doc *) $a_doc, (const int32_t *) $a_ranges, (int32_t) $a_range_count, (int32_t *) $a_count);"
		end

	c_doc_bracket_pair (a_doc: POINTER; a_position: INTEGER): POINTER
		external
			"C blocking inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_bracket_pair((sts_doc *) $a_doc, (int32_t) $a_position);"
		end

	c_doc_changed_lines (a_doc: POINTER; a_count: TYPED_POINTER [INTEGER]): POINTER
		external
			"C blocking inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_changed_lines((sts_doc *) $a_doc, (int32_t *) $a_count);"
		end

	c_doc_changes_taken (a_doc: POINTER)
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"sts_doc_changes_taken((sts_doc *) $a_doc);"
		end

	c_doc_folds (a_doc: POINTER; a_count: TYPED_POINTER [INTEGER]): POINTER
		external
			"C blocking inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_folds((sts_doc *) $a_doc, (int32_t *) $a_count);"
		end

	c_doc_bracket_at (a_doc: POINTER; a_position: INTEGER; a_pair: POINTER): BOOLEAN
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_BOOLEAN) (sts_doc_bracket_at((sts_doc *) $a_doc, (int32_t) $a_position, (int32_t *) $a_pair) != 0);"
		end

	c_doc_was_limited (a_doc: POINTER): BOOLEAN
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_BOOLEAN) (sts_doc_was_limited((const sts_doc *) $a_doc) != 0);"
		end

feature {NONE} -- Trees

	c_doc_sexp (a_doc: POINTER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_sexp((sts_doc *) $a_doc);"
		end

	c_doc_node_at (a_doc: POINTER; a_position: INTEGER; an_info: POINTER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_node_at((sts_doc *) $a_doc, (int32_t) $a_position, (int32_t *) $an_info);"
		end

	c_cursor_new (a_doc: POINTER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_cursor_new((sts_doc *) $a_doc);"
		end

	c_cursor_free (a_cursor: POINTER)
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"sts_cursor_free((sts_cursor *) $a_cursor);"
		end

	c_cursor_goto_first_child (a_cursor: POINTER): BOOLEAN
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_BOOLEAN) (sts_cursor_goto_first_child((sts_cursor *) $a_cursor) != 0);"
		end

	c_cursor_goto_next_sibling (a_cursor: POINTER): BOOLEAN
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_BOOLEAN) (sts_cursor_goto_next_sibling((sts_cursor *) $a_cursor) != 0);"
		end

	c_cursor_goto_parent (a_cursor: POINTER): BOOLEAN
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_BOOLEAN) (sts_cursor_goto_parent((sts_cursor *) $a_cursor) != 0);"
		end

	c_cursor_node (a_cursor: POINTER; an_info: POINTER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_cursor_node((sts_cursor *) $a_cursor, (int32_t *) $an_info);"
		end

feature {NONE} -- Queries

	c_query_new (a_language, a_source: POINTER; a_length: INTEGER; an_error_offset, an_error_kind: TYPED_POINTER [INTEGER]; a_message: POINTER; a_capacity: INTEGER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_query_new((const char *) $a_language, (const char *) $a_source, (int32_t) $a_length, (int32_t *) $an_error_offset, (int32_t *) $an_error_kind, (char *) $a_message, (int32_t) $a_capacity);"
		end

	c_query_free (a_query: POINTER)
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"sts_query_free((sts_query *) $a_query);"
		end

	c_query_capture_count (a_query: POINTER): INTEGER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_INTEGER) sts_query_capture_count((const sts_query *) $a_query);"
		end

	c_query_capture_name (a_query: POINTER; an_index: INTEGER): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_query_capture_name((const sts_query *) $a_query, (int32_t) $an_index);"
		end

	c_query_pattern_count (a_query: POINTER): INTEGER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_INTEGER) sts_query_pattern_count((const sts_query *) $a_query);"
		end

	c_doc_run (a_doc, a_query: POINTER; a_first, a_last: INTEGER; a_count: TYPED_POINTER [INTEGER]): POINTER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_POINTER) sts_doc_run((sts_doc *) $a_doc, (sts_query *) $a_query, (int32_t) $a_first, (int32_t) $a_last, (int32_t *) $a_count);"
		end

feature {NONE} -- Memory

	c_free (a_block: POINTER)
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"sts_free((void *) $a_block);"
		end

	c_live_blocks: INTEGER
		external
			"C inline use %"simple_tree_sitter.h%""
		alias
			"return (EIF_INTEGER) sts_live_blocks();"
		end

	c_int_at (a_block: POINTER; an_index: INTEGER): INTEGER
			-- The 32-bit integer at `an_index' (from 0) of `a_block'.
		external
			"C inline use <stdint.h>"
		alias
			"return (EIF_INTEGER) ((const int32_t *) $a_block)[$an_index];"
		end

end
