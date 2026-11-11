note
	description: "[
		One text and its syntax tree, in one of the library's languages.

		Positions count characters (Unicode code points) from 1, lines from 1,
		and a column is a character's place in its line, from 1. A line ends at
		a line feed; the line feed belongs to the line it ends.

		A document belongs to one processor. Every call parses or reads on the
		caller's own processor, which suits tools, tests and small files; an
		editor keeps its document on a worker (see the README).
	]"

class
	TS_DOCUMENT

inherit
	TS_BLOCKS

	DISPOSABLE

create
	make

feature {NONE} -- Initialization

	make (a_language: READABLE_STRING_GENERAL)
			-- An empty text in `a_language'.
		require
			language_known: (create {TS_LANGUAGES}).has (a_language)
		local
			l_name: C_STRING
		do
			language := a_language.to_string_8
			create l_name.make (language)
			handle := c_doc_new (l_name.item)
		ensure
			language_set: language.same_string_general (a_language)
			empty: count = 0
			one_line: line_count = 1
			version_zero: version = 0
		end

feature -- Access

	language: STRING
			-- The language of the text.

	languages: TS_LANGUAGES
			-- The languages a document can be made in.
		once
			create Result
		end

	count: INTEGER
			-- The number of characters.
		do
			Result := c_doc_count (handle)
		ensure
			not_negative: Result >= 0
		end

	line_count: INTEGER
			-- The number of lines: one more than the number of line feeds.
		do
			Result := c_doc_line_count (handle)
		end

	line_start (a_line: INTEGER): INTEGER
			-- The position of the first character of `a_line'; `count' + 1 for a last
			-- line that is empty.
		require
			line_in_text: 1 <= a_line and a_line <= line_count
		do
			Result := c_doc_line_start (handle, a_line - 1) + 1
		ensure
			in_text: 1 <= Result and Result <= count + 1
		end

	version: INTEGER
			-- How many times the text has been set or edited.

	text: STRING_32
			-- A copy of the text.
		local
			l_text: POINTER
			i, n: INTEGER
		do
			n := count
			create Result.make (n)
			l_text := c_doc_text (handle)
			from
				i := 0
			until
				i >= n
			loop
				Result.append_code (c_int_at (l_text, i).to_natural_32)
				i := i + 1
			end
		ensure
			whole_text: Result.count = count
		end

feature -- Status report

	has_error: BOOLEAN
			-- Is there text the parser could not place, or text it had to make up?
		do
			Result := c_doc_has_error (handle)
		end

	was_limited: BOOLEAN
			-- Did a limit cut short the last call of `spans', `folds', `bracket_at' or
			-- `captures'? What it answered is then right as far as it goes, and may
			-- lack items.
		do
			Result := c_doc_was_limited (handle)
		end

feature -- Element change

	set_text (a_text: READABLE_STRING_32)
			-- Replace the whole text with `a_text' and parse it from nothing.
		local
			l_text: STRING_32
			l_area: ANY
		do
			l_text := a_text.to_string_32
			l_area := l_text.to_c
			c_doc_set_text (handle, $l_area, l_text.count)
			version := version + 1
		ensure
			count_set: count = a_text.count
			one_more: version = old version + 1
		end

	edit (a_start, a_removed: INTEGER; an_inserted: READABLE_STRING_32)
			-- Put `an_inserted' in place of the `a_removed' characters from position
			-- `a_start', and parse again, reusing the tree for the text that stayed.
			-- `a_start' = `count' + 1 with nothing removed adds at the end.
		require
			start_in_text: 1 <= a_start and a_start <= count + 1
			removed_in_text: 0 <= a_removed and a_removed <= count - a_start + 1
		local
			l_text: STRING_32
			l_area: ANY
			l_done: BOOLEAN
		do
			l_text := an_inserted.to_string_32
			l_area := l_text.to_c
			l_done := c_doc_edit (handle, a_start - 1, a_removed, $l_area, l_text.count)
			check range_was_in_text: l_done end
			version := version + 1
		ensure
			count_set: count = old count - a_removed + an_inserted.count
			one_more: version = old version + 1
		end

	set_text_block (a_block: POINTER; a_count: INTEGER)
			-- As `set_text', from the `a_count' characters of `a_block', a block made by
			-- `new_text_block'. Frees the block.
		require
			count_not_negative: a_count >= 0
			null_is_empty: a_block = default_pointer implies a_count = 0
		do
			c_doc_set_text (handle, a_block, a_count)
			c_free (a_block)
			version := version + 1
		ensure
			count_set: count = a_count
			one_more: version = old version + 1
		end

	edit_block (a_start, a_removed: INTEGER; a_block: POINTER; a_count: INTEGER)
			-- As `edit', inserting the `a_count' characters of `a_block', a block made by
			-- `new_text_block'. Frees the block.
		require
			start_in_text: 1 <= a_start and a_start <= count + 1
			removed_in_text: 0 <= a_removed and a_removed <= count - a_start + 1
			count_not_negative: a_count >= 0
			null_is_empty: a_block = default_pointer implies a_count = 0
		local
			l_done: BOOLEAN
		do
			l_done := c_doc_edit (handle, a_start - 1, a_removed, a_block, a_count)
			c_free (a_block)
			check range_was_in_text: l_done end
			version := version + 1
		ensure
			count_set: count = old count - a_removed + a_count
			one_more: version = old version + 1
		end

feature -- Highlighting

	spans (a_first_line, a_last_line: INTEGER): TS_SPANS
			-- The highlight spans of lines `a_first_line' to `a_last_line', from the
			-- language's highlights query. Lines past the end of the text have none.
		require
			first_positive: a_first_line >= 1
		local
			l_block: POINTER
			n: INTEGER
		do
			l_block := c_doc_spans (handle, a_first_line - 1, a_last_line - 1, $n)
			create Result.make_from_block (l_block, n)
		end

	changed_lines: TS_LINE_RANGES
			-- Every range of lines whose spans may differ from the ones last answered,
			-- in the line numbers of the text as it is now. Asking forgets nothing: see
			-- `forget_changed_lines'.
		local
			l_block: POINTER
			n: INTEGER
		do
			l_block := c_doc_changed_lines (handle, $n)
			create Result.make_from_block (l_block, n)
		end

	forget_changed_lines
			-- Forget the ranges the last `changed_lines' answered. Lines changed since
			-- that call stay owed.
		do
			c_doc_changes_taken (handle)
		end

feature -- Structure

	folds: TS_LINE_RANGES
			-- The ranges of lines that can fold, from the language's folds query: in
			-- order of first line, each over at least two lines.
		local
			l_block: POINTER
			n: INTEGER
		do
			l_block := c_doc_folds (handle, $n)
			create Result.make_from_block (l_block, n)
		end

	bracket_at (a_position: INTEGER): detachable TS_BRACKET_PAIR
			-- The bracket pair that the character at `a_position', or failing that the
			-- one before it, is an end of, from the language's brackets query. Void when
			-- neither is, or when the other end is not in the text.
		require
			position_in_text: 1 <= a_position and a_position <= count + 1
		local
			l_pair: MANAGED_POINTER
		do
			create l_pair.make (16)
			if
				c_doc_bracket_at (handle, a_position - 1, l_pair.item)
				and then l_pair.read_integer_32 (4) > 0 and then l_pair.read_integer_32 (12) > 0
			then
				create Result.make (l_pair.read_integer_32 (0) + 1, l_pair.read_integer_32 (4),
					l_pair.read_integer_32 (8) + 1, l_pair.read_integer_32 (12))
			end
		end

feature -- Trees, for tools and tests

	node_at (a_position: INTEGER): TS_NODE_INFO
			-- The smallest node over the character at `a_position'; for white space
			-- that no node covers, the node around it.
		require
			position_in_text: 1 <= a_position and a_position <= count
		local
			l_info: MANAGED_POINTER
			l_type: POINTER
		do
			create l_info.make (28)
			l_type := c_doc_node_at (handle, a_position - 1, l_info.item)
			check has_tree: l_type /= default_pointer then
				create Result.make_from_c (l_type, l_info)
			end
		end

	s_expression: STRING
			-- The tree written out as an S-expression, as tree-sitter's own tools show it.
		local
			l_text: POINTER
		do
			l_text := c_doc_sexp (handle)
			if l_text = default_pointer then
				create Result.make_empty
			else
				create Result.make_from_c (l_text)
				c_free (l_text)
			end
		end

	new_cursor: TS_CURSOR
			-- A cursor on the root of the tree, good until the text next changes.
		do
			create Result.make (Current, c_cursor_new (handle))
		ensure
			valid: Result.is_valid
			at_this_version: Result.version = version
		end

	captures (a_query: TS_QUERY; a_first, a_last: INTEGER): TS_CAPTURES
			-- What `a_query' captures in the nodes that touch the characters `a_first'
			-- to `a_last'. Positions past the end of the text have none.
		require
			query_valid: a_query.is_valid
			same_language: a_query.language.same_string (language)
			first_positive: a_first >= 1
		local
			l_block: POINTER
			n: INTEGER
		do
			l_block := c_doc_run (handle, a_query.handle, a_first - 1, a_last - 1, $n)
			create Result.make (l_block, n, a_query.capture_names)
		end

feature {NONE} -- Implementation

	handle: POINTER
			-- The C document.

	dispose
			-- Free the C document.
		do
			if handle /= default_pointer then
				c_doc_free (handle)
				handle := default_pointer
			end
		end

invariant
	version_not_negative: version >= 0

end
