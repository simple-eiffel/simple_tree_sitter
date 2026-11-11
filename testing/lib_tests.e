note
	description: "[
		Tests of the Eiffel classes over the C wrapper. The C side has its own
		tests (Clib/test/sts_test.c); these check that the Eiffel classes hand
		positions, lines and blocks across correctly, and that their contracts
		hold on real text.
	]"
	testing: "covers"

class
	LIB_TESTS

inherit
	TEST_SET_BASE

feature -- Test: positions

	test_positions_count_characters
			-- Positions, columns and lengths count characters (Unicode code points),
			-- not bytes and not UTF-16 units: the sample has an accented letter, a
			-- mathematical sign and an emoji outside the first 65,536 characters, in
			-- a comment and in a string.
		local
			l_doc: TS_DOCUMENT
			l_spans: TS_SPANS
			l_text: STRING_32
			l_start, l_string: INTEGER
			l_node: TS_NODE_INFO
		do
			l_text := sample
			create l_doc.make ("eiffel")
			l_doc.set_text (l_text)
			assert_integers_equal ("count is the number of characters", l_text.count, l_doc.count)
			assert_integers_equal ("ten lines: nine, and the empty one after the last line feed", 10, l_doc.line_count)
			assert_integers_equal ("line 1 starts at 1", 1, l_doc.line_start (1))
			assert_integers_equal ("line 2 starts after 11 characters and a line feed", 13, l_doc.line_start (2))
			assert_integers_equal ("the last line starts after the text", l_text.count + 1, l_doc.line_start (10))
			assert_true ("text comes back as it went in", l_doc.text.same_string (l_text))
			assert_false ("the sample parses clean", l_doc.has_error)

			l_spans := l_doc.spans (1, l_doc.line_count)
			assert_spans_sound ("sample", l_doc)
			assert_integers_equal ("19 spans", 19, l_spans.count)
			assert_integers_equal ("comment line", 1, l_spans.line (1))
			assert_integers_equal ("comment column", 1, l_spans.column (1))
			assert_integers_equal ("comment length: 11 characters", 11, l_spans.length (1))
			assert_integers_equal ("comment style", {TS_STYLES}.Style_comment, l_spans.style (1))

			l_string := span_with_style (l_spans, {TS_STYLES}.Style_string)
			assert_true ("a string span", l_string > 0)
			assert_integers_equal ("string line", 6, l_spans.line (l_string))
			assert_integers_equal ("string column", 14, l_spans.column (l_string))
			assert_integers_equal ("string length: 17 characters with its quotes", 17, l_spans.length (l_string))
			l_start := l_doc.line_start (6) + l_spans.column (l_string) - 1
			assert_true ("the span is the string",
				l_text.substring (l_start, l_start + l_spans.length (l_string) - 1).same_string (sample_string))

				-- The emoji is the tenth character inside the quotes.
			l_node := l_doc.node_at (l_start + 10)
			assert_true ("the emoji is in the string's content", l_node.type.same_string ("string_content"))
			assert_integers_equal ("content starts after the quote", l_start + 1, l_node.start)
			assert_integers_equal ("content length", 15, l_node.length)
			assert_integers_equal ("content first line", 6, l_node.first_line)
			assert_integers_equal ("content last line", 6, l_node.last_line)
			assert_false ("no limit was hit", l_doc.was_limited)
		end

feature -- Test: languages and styles

	test_languages
		local
			l_languages: TS_LANGUAGES
			l_names: LIST [STRING]
		do
			create l_languages
			l_names := l_languages.names
			assert_integers_equal ("four languages", 4, l_names.count)
			assert_true ("eiffel", l_languages.has ("eiffel"))
			assert_true ("c", l_languages.has ("c"))
			assert_true ("json", l_languages.has ("json"))
			assert_true ("xml", l_languages.has ("xml"))
			assert_false ("not pascal", l_languages.has ("pascal"))
			assert_false ("not the empty name", l_languages.has (""))
			assert_false ("names are lower case", l_languages.has ("Eiffel"))
			assert_true ("a name outside Latin-1 is no language", not l_languages.has ({STRING_32} "%/0x2200/"))

			assert_true (".e", attached l_languages.name_for_file ("src/ts_document.e") as al_name and then al_name.same_string ("eiffel"))
			assert_true (".ECF, any case", attached l_languages.name_for_file ("C:\work\simple.ECF") as al_name and then al_name.same_string ("xml"))
			assert_true (".h", attached l_languages.name_for_file ("simple_tree_sitter.h") as al_name and then al_name.same_string ("c"))
			assert_true (".json", attached l_languages.name_for_file ("a.b/package.json") as al_name and then al_name.same_string ("json"))
			assert_true ("no extension", l_languages.name_for_file ("README") = Void)
			assert_true ("a dot in a directory is not an extension", l_languages.name_for_file ("a.e/README") = Void)
			assert_true ("unknown extension", l_languages.name_for_file ("notes.txt") = Void)
			assert_true ("a dot at the end", l_languages.name_for_file ("notes.") = Void)
			assert_true ("the empty path", l_languages.name_for_file ("") = Void)
		end

	test_styles
		local
			l_styles: TS_STYLES
			i: INTEGER
		do
			create l_styles
			assert_integers_equal ("keyword", {TS_STYLES}.Style_keyword, l_styles.for_capture ("keyword"))
			assert_integers_equal ("keyword.control", {TS_STYLES}.Style_keyword, l_styles.for_capture ("keyword.control"))
			assert_integers_equal ("character is a string", {TS_STYLES}.Style_string, l_styles.for_capture ("character"))
			assert_integers_equal ("boolean is a number", {TS_STYLES}.Style_number, l_styles.for_capture ("boolean"))
			assert_integers_equal ("punctuation.bracket", {TS_STYLES}.Style_operator, l_styles.for_capture ("punctuation.bracket"))
			assert_integers_equal ("property", {TS_STYLES}.Style_variable, l_styles.for_capture ("property"))
			assert_integers_equal ("attribute", {TS_STYLES}.Style_tag, l_styles.for_capture ("attribute"))
			assert_integers_equal ("an unknown name is plain", {TS_STYLES}.Style_plain, l_styles.for_capture ("nothing.known"))
			assert_integers_equal ("a longer word is not the shorter one", {TS_STYLES}.Style_plain, l_styles.for_capture ("keywords"))
			assert_integers_equal ("the first seven are GUI_CODE_EDITOR's", 6, {TS_STYLES}.Style_operator)
			from
				i := {TS_STYLES}.Style_plain
			until
				i > {TS_STYLES}.Style_last
			loop
				assert_true ("style " + i.out + " has a name", not l_styles.name (i).is_empty)
				i := i + 1
			end
		end

feature -- Test: documents

	test_empty_document
		local
			l_doc: TS_DOCUMENT
			l_cursor: TS_CURSOR
		do
			create l_doc.make ("eiffel")
			assert_integers_equal ("no characters", 0, l_doc.count)
			assert_integers_equal ("one line", 1, l_doc.line_count)
			assert_integers_equal ("which starts at 1", 1, l_doc.line_start (1))
			assert_integers_equal ("version 0", 0, l_doc.version)
			assert_true ("no spans", l_doc.spans (1, 1).is_empty)
			assert_true ("no folds", l_doc.folds.is_empty)
			assert_true ("no bracket", l_doc.bracket_at (1) = Void)
			assert_true ("no text", l_doc.text.is_empty)
			assert_false ("a tree all the same", l_doc.s_expression.is_empty)
			l_cursor := l_doc.new_cursor
			assert_integers_equal ("the root covers nothing", 0, l_cursor.node.length)

			l_doc.set_text ({STRING_32} "")
			assert_integers_equal ("still no characters", 0, l_doc.count)
			assert_integers_equal ("version 1", 1, l_doc.version)
			l_doc.edit (1, 0, {STRING_32} "class A end")
			assert_integers_equal ("eleven characters", 11, l_doc.count)
			assert_integers_equal ("version 2", 2, l_doc.version)
			assert_false ("three spans", l_doc.spans (1, 1).count /= 3)
			l_doc.edit (1, 11, {STRING_32} "")
			assert_integers_equal ("empty again", 0, l_doc.count)
			assert_integers_equal ("one line again", 1, l_doc.line_count)
		end

	test_spans_of_a_line_range
		local
			l_doc: TS_DOCUMENT
			l_spans: TS_SPANS
			i: INTEGER
		do
			l_doc := new_sample_document
			l_spans := l_doc.spans (3, 4)
			assert_integers_equal ("lines 3 and 4 have four spans", 4, l_spans.count)
			from
				i := 1
			until
				i > l_spans.count
			loop
				assert_true ("in the range asked for", l_spans.line (i) = 3 or l_spans.line (i) = 4)
				i := i + 1
			end
			assert_integers_equal ("feature", 7, l_spans.length (1))
			assert_integers_equal ("a keyword", {TS_STYLES}.Style_keyword, l_spans.style (1))
			assert_integers_equal ("greeting, after a tab", 2, l_spans.column (2))
			assert_integers_equal ("a function", {TS_STYLES}.Style_function, l_spans.style (2))
			assert_integers_equal ("STRING_32 is a type", {TS_STYLES}.Style_type, l_spans.style (4))
			assert_true ("one line", l_doc.spans (9, 9).count = 1)
			assert_true ("nothing past the end", l_doc.spans (50, 60).is_empty)
			assert_true ("nothing in an empty range", l_doc.spans (5, 2).is_empty)
			assert_true ("the range is cut to the text", l_doc.spans (9, 1000).count = 1)
			assert_true ("valid_index agrees with count", l_spans.valid_index (4) and not l_spans.valid_index (5) and not l_spans.valid_index (0))
		end

	test_folds
		local
			l_folds: TS_LINE_RANGES
		do
			l_folds := new_sample_document.folds
			assert_integers_equal ("class, feature clause, routine", 3, l_folds.count)
			assert_integers_equal ("class from", 2, l_folds.first_line (1))
			assert_integers_equal ("class to", 9, l_folds.last_line (1))
			assert_integers_equal ("feature clause from", 3, l_folds.first_line (2))
			assert_integers_equal ("feature clause to", 8, l_folds.last_line (2))
			assert_integers_equal ("routine from", 4, l_folds.first_line (3))
			assert_integers_equal ("routine to", 8, l_folds.last_line (3))
			assert_true ("line 5 is in a fold", l_folds.has_line (5))
			assert_false ("line 1 is in none", l_folds.has_line (1))
		end

	test_brackets
		local
			l_doc: TS_DOCUMENT
			l_line: INTEGER
		do
			l_doc := new_sample_document
				-- Line 7 is three tabs and "print (Result [1])".
			l_line := l_doc.line_start (7)
			assert_pair ("on the round bracket", l_doc.bracket_at (l_line + 9), l_line + 9, 1, l_line + 20, 1)
			assert_pair ("on its other end", l_doc.bracket_at (l_line + 20), l_line + 9, 1, l_line + 20, 1)
			assert_pair ("just after it: the character before counts", l_doc.bracket_at (l_line + 21), l_line + 9, 1, l_line + 20, 1)
			assert_pair ("on the square bracket", l_doc.bracket_at (l_line + 17), l_line + 17, 1, l_line + 19, 1)
			assert_pair ("do and its end", l_doc.bracket_at (l_doc.line_start (5) + 2), l_doc.line_start (5) + 2, 2, l_doc.line_start (8) + 2, 3)
			assert_pair ("from the end", l_doc.bracket_at (l_doc.line_start (8) + 3), l_doc.line_start (5) + 2, 2, l_doc.line_start (8) + 2, 3)
			assert_pair ("class and its end", l_doc.bracket_at (l_doc.line_start (2)), l_doc.line_start (2), 5, l_doc.line_start (9), 3)
			assert_true ("none in a comment", l_doc.bracket_at (1) = Void)
			assert_true ("none on a name", l_doc.bracket_at (l_line + 5) = Void)
			assert_pair ("just after the last end", l_doc.bracket_at (l_doc.line_start (9) + 3), l_doc.line_start (2), 5, l_doc.line_start (9), 3)
			assert_true ("none after the last line feed", l_doc.bracket_at (l_doc.count + 1) = Void)
		end

	test_node_at
		local
			l_doc: TS_DOCUMENT
			l_node: TS_NODE_INFO
		do
			l_doc := new_sample_document
			l_node := l_doc.node_at (l_doc.line_start (2) + 6)
			assert_true ("the class name", l_node.type.same_string ("class_name"))
			assert_integers_equal ("where it is", l_doc.line_start (2) + 6, l_node.start)
			assert_integers_equal ("one character", 1, l_node.length)
			assert_integers_equal ("on line 2", 2, l_node.first_line)
			assert_integers_equal ("only", 2, l_node.last_line)
			assert_true ("a named node", l_node.is_named)
			assert_false ("no error", l_node.is_error or l_node.is_missing)

			l_node := l_doc.node_at (l_doc.line_start (2))
			assert_true ("a keyword's type is its text", l_node.type.same_string ("class"))
			assert_false ("and it is not named", l_node.is_named)
			assert_integers_equal ("five characters", 5, l_node.length)

			l_node := l_doc.node_at (1)
			assert_true ("the comment", l_node.type.same_string ("comment"))
			assert_integers_equal ("its length", 11, l_node.length)

			l_doc.set_text ({STRING_32} "class A feature x: INTEGER do Result := ( end end")
			assert_true ("a text with an unclosed bracket has an error", l_doc.has_error)
			l_node := l_doc.node_at (l_doc.count)
			assert_true ("and still answers nodes", not l_node.type.is_empty)
		end

	test_cursor
		local
			l_doc: TS_DOCUMENT
			l_cursor: TS_CURSOR
		do
			l_doc := new_sample_document
			l_cursor := l_doc.new_cursor
			assert_true ("valid", l_cursor.is_valid)
			assert_true ("on the root", l_cursor.node.type.same_string ("source_file"))
			l_cursor.go_to_parent
			assert_false ("the root has no parent", l_cursor.has_moved)
			assert_true ("so the cursor stays", l_cursor.node.type.same_string ("source_file"))
			l_cursor.go_to_first_child
			assert_true ("down", l_cursor.has_moved)
			assert_true ("to the comment", l_cursor.node.type.same_string ("comment"))
			l_cursor.go_to_first_child
			assert_false ("a comment has no children", l_cursor.has_moved)
			l_cursor.go_to_next_sibling
			assert_true ("along", l_cursor.has_moved)
			assert_true ("to the class", l_cursor.node.type.same_string ("class_declaration"))
			assert_integers_equal ("which starts on line 2", 2, l_cursor.node.first_line)
			assert_integers_equal ("and ends on line 9", 9, l_cursor.node.last_line)
			l_cursor.go_to_next_sibling
			assert_false ("the class is the last", l_cursor.has_moved)
			l_cursor.go_to_parent
			assert_true ("up", l_cursor.has_moved)
			assert_true ("to the root", l_cursor.node.type.same_string ("source_file"))

			l_doc.edit (1, 0, {STRING_32} " ")
			assert_false ("an edit ends the cursor's life", l_cursor.is_valid)
			assert_true ("a new one is valid", l_doc.new_cursor.is_valid)
		end

feature -- Test: edits

	test_edit_agrees_with_a_fresh_parse
			-- After an edit the document answers what a document made from the whole
			-- new text answers.
		local
			l_doc: TS_DOCUMENT
			l_text: STRING_32
			l_at: INTEGER
		do
			l_text := sample
			l_doc := new_sample_document
			assert_integers_equal ("version 1 after the first text", 1, l_doc.version)

				-- An emoji and a space at the front of the string's content.
			l_at := l_doc.line_start (6) + 14
			l_doc.edit (l_at, 0, {STRING_32} "%/0x1F600/ ")
			l_text.insert_string ({STRING_32} "%/0x1F600/ ", l_at)
			assert_integers_equal ("version 2", 2, l_doc.version)
			assert_same_as_fresh ("after an insertion", l_doc, l_text)
			assert_integers_equal ("the string grew by two", 19, l_doc.spans (6, 6).length (3))

				-- A line taken out: the whole of line 7 with its line feed.
			l_at := l_doc.line_start (7)
			l_doc.edit (l_at, l_doc.line_start (8) - l_at, {STRING_32} "")
			l_text.remove_substring (l_at, l_at + (l_text.index_of ('%N', l_at) - l_at))
			assert_same_as_fresh ("after a removal", l_doc, l_text)
			assert_integers_equal ("one line fewer", 9, l_doc.line_count)

				-- A replacement that breaks the text, then one that mends it.
			l_at := l_doc.line_start (5) + 2
			l_doc.edit (l_at, 2, {STRING_32} "(((")
			l_text.replace_substring ({STRING_32} "(((", l_at, l_at + 1)
			assert_same_as_fresh ("after a breaking change", l_doc, l_text)
			assert_true ("it is broken", l_doc.has_error)
			l_doc.edit (l_at, 3, {STRING_32} "do")
			l_text.replace_substring ({STRING_32} "do", l_at, l_at + 2)
			assert_same_as_fresh ("after the mend", l_doc, l_text)
			assert_false ("it is whole", l_doc.has_error)

				-- At the very end.
			l_doc.edit (l_doc.count + 1, 0, {STRING_32} "-- the end%N")
			l_text.append ({STRING_32} "-- the end%N")
			assert_same_as_fresh ("after adding at the end", l_doc, l_text)
			assert_integers_equal ("six changes", 6, l_doc.version)
		end

	test_changed_lines
		local
			l_doc: TS_DOCUMENT
			l_changed: TS_LINE_RANGES
		do
			l_doc := new_sample_document
			l_changed := l_doc.changed_lines
			assert_integers_equal ("a new text is owed whole: one range", 1, l_changed.count)
			assert_integers_equal ("from line 1", 1, l_changed.first_line (1))
			assert_integers_equal ("to the last", l_doc.line_count, l_changed.last_line (1))
			assert_integers_equal ("asking forgets nothing", 1, l_doc.changed_lines.count)
			l_doc.forget_changed_lines
			assert_true ("taken: nothing is owed", l_doc.changed_lines.is_empty)

				-- A change inside the string on line 6.
			l_doc.edit (l_doc.line_start (6) + 14, 1, {STRING_32} "N")
			l_changed := l_doc.changed_lines
			assert_true ("line 6 is owed", l_changed.has_line (6))
			assert_false ("line 2 is not", l_changed.has_line (2))

				-- A second change before the first was taken: both stay owed.
			l_doc.edit (l_doc.line_start (2) + 6, 1, {STRING_32} "B")
			l_changed := l_doc.changed_lines
			assert_true ("line 6 is still owed", l_changed.has_line (6))
			assert_true ("and line 2 now", l_changed.has_line (2))
			l_doc.forget_changed_lines
			assert_true ("taken again", l_doc.changed_lines.is_empty)

				-- A change after an answer and before it is taken stays owed.
			l_doc.edit (l_doc.line_start (6) + 14, 1, {STRING_32} "n")
			l_changed := l_doc.changed_lines
			l_doc.edit (l_doc.line_start (2) + 6, 1, {STRING_32} "A")
			l_doc.forget_changed_lines
			l_changed := l_doc.changed_lines
			assert_true ("the later change is still owed", l_changed.has_line (2))
			assert_false ("the answered one is not", l_changed.has_line (6))
		end

	test_text_blocks
			-- The block calls do what the string calls do, and free what they are given.
			-- The count of live blocks is the whole process's, results waiting for
			-- the collector included, so it is read anew before each block is made.
		local
			l_doc, l_other: TS_DOCUMENT
			l_text: STRING_32
			l_block: POINTER
			l_live: INTEGER
		do
			l_text := sample
			create l_doc.make ("eiffel")
			l_live := l_doc.live_block_count
			l_block := l_doc.new_text_block (l_text)
			assert_true ("a block", l_block /= default_pointer)
			assert_integers_equal ("one more block is live", l_live + 1, l_doc.live_block_count)
			l_doc.set_text_block (l_block, l_text.count)
			assert_integers_equal ("the document freed it", l_live, l_doc.live_block_count)
			assert_integers_equal ("version 1", 1, l_doc.version)
			assert_same_as_fresh ("after set_text_block", l_doc, l_text)

			l_live := l_doc.live_block_count
			l_block := l_doc.new_text_block ({STRING_32} "%/0x2200/x")
			assert_integers_equal ("one more again", l_live + 1, l_doc.live_block_count)
			l_doc.edit_block (l_doc.line_start (6) + 14, 1, l_block, 2)
			l_text.replace_substring ({STRING_32} "%/0x2200/x", l_doc.line_start (6) + 14, l_doc.line_start (6) + 14)
			assert_integers_equal ("freed again", l_live, l_doc.live_block_count)
			assert_integers_equal ("version 2", 2, l_doc.version)
			assert_same_as_fresh ("after edit_block", l_doc, l_text)

			assert_true ("an empty text has no block", l_doc.new_text_block ({STRING_32} "") = default_pointer)
			l_doc.edit_block (1, 0, default_pointer, 0)
			assert_same_as_fresh ("an empty edit changes nothing", l_doc, l_text)
			assert_integers_equal ("but counts as a version", 3, l_doc.version)
			l_doc.set_text_block (default_pointer, 0)
			assert_integers_equal ("an empty block empties the text", 0, l_doc.count)

			l_live := l_doc.live_block_count
			l_block := l_doc.new_text_block (l_text)
			l_doc.free_block (l_block)
			assert_integers_equal ("a block handed to nothing is freed by hand", l_live, l_doc.live_block_count)

				-- A block made with one object is good for another.
			create l_other.make ("eiffel")
			l_live := l_doc.live_block_count
			l_other.set_text_block (l_doc.new_text_block (l_text), l_text.count)
			assert_integers_equal ("freed by the other document", l_live, l_doc.live_block_count)
			assert_same_as_fresh ("in the other document", l_other, l_text)
		end

feature -- Test: queries

	test_query
		local
			l_doc: TS_DOCUMENT
			l_query: TS_QUERY
			l_captures: TS_CAPTURES
		do
			l_doc := new_sample_document
			create l_query.make ("eiffel", "(class_name) @name")
			assert_true ("valid", l_query.is_valid)
			assert_true ("no message", l_query.error_message.is_empty)
			assert_integers_equal ("no position", 0, l_query.error_position)
			assert_integers_equal ("one pattern", 1, l_query.pattern_count)
			assert_integers_equal ("one capture name", 1, l_query.capture_names.count)
			assert_true ("name", l_query.capture_names.first.same_string ("name"))
			assert_true ("for eiffel", l_query.language.same_string ("eiffel"))

			l_captures := l_doc.captures (l_query, 1, l_doc.count)
			assert_integers_equal ("A and STRING_32", 2, l_captures.count)
			assert_integers_equal ("A is where it is", l_doc.line_start (2) + 6, l_captures.start (1))
			assert_integers_equal ("A is one character", 1, l_captures.length (1))
			assert_true ("it is the name capture", l_captures.name (1).same_string ("name"))
			assert_integers_equal ("of pattern 1", 1, l_captures.pattern (1))
			assert_integers_equal ("STRING_32 is where it is", l_doc.line_start (4) + 11, l_captures.start (2))
			assert_integers_equal ("STRING_32 is nine characters", 9, l_captures.length (2))

			assert_integers_equal ("only what touches the range", 1, l_doc.captures (l_query, 1, l_doc.line_start (3)).count)
			assert_true ("nothing in the comment", l_doc.captures (l_query, 1, 11).is_empty)
			assert_true ("nothing past the end", l_doc.captures (l_query, l_doc.count + 5, l_doc.count + 9).is_empty)
			assert_true ("nothing in an empty range", l_doc.captures (l_query, 9, 3).is_empty)

				-- Two patterns, two names, and a predicate.
			create l_query.make ("eiffel", "((identifier) @call (#eq? @call %"print%"))%N(integer) @number")
			assert_true ("valid with a predicate", l_query.is_valid)
			assert_integers_equal ("two patterns", 2, l_query.pattern_count)
			l_captures := l_doc.captures (l_query, 1, l_doc.count)
			assert_integers_equal ("print and 1, not greeting", 2, l_captures.count)
			assert_true ("print first", l_captures.name (1).same_string ("call"))
			assert_integers_equal ("five characters", 5, l_captures.length (1))
			assert_true ("then the number", l_captures.name (2).same_string ("number"))
			assert_integers_equal ("of pattern 2", 2, l_captures.pattern (2))
		end

	test_query_errors
		local
			l_query: TS_QUERY
		do
			create l_query.make ("eiffel", "(no_such_node) @x")
			assert_false ("an unknown node type", l_query.is_valid)
			assert_false ("with a message", l_query.error_message.is_empty)
			assert_integers_equal ("at the name", 2, l_query.error_position)
			assert_true ("no capture names", l_query.capture_names.is_empty)

				-- The position counts characters: the comment line has an accented
				-- letter and an emoji, each one character and several bytes.
			create l_query.make ("eiffel", {STRING_32} "; caf%/0xE9/ %/0x1F600/%N(no_such_node) @x")
			assert_false ("still invalid", l_query.is_valid)
			assert_integers_equal ("9 characters and the bracket before the name", 11, l_query.error_position)

			create l_query.make ("eiffel", "(class_name")
			assert_false ("an unclosed bracket", l_query.is_valid)
			assert_true ("a position in the source", l_query.error_position >= 1 and l_query.error_position <= 11)

			create l_query.make ("eiffel", "((identifier) @x (#is-not? local))")
			assert_false ("a predicate the library does not have", l_query.is_valid)
			assert_false ("says so", l_query.error_message.is_empty)

			create l_query.make ("json", "(class_name) @x")
			assert_false ("a node of another language", l_query.is_valid)
			create l_query.make ("json", "")
			assert_true ("an empty query is valid", l_query.is_valid)
			assert_integers_equal ("and has no patterns", 0, l_query.pattern_count)
		end

feature -- Test: the other languages

	test_other_languages
		local
			l_doc: TS_DOCUMENT
			l_spans: TS_SPANS
		do
			create l_doc.make ("json")
			l_doc.set_text ({STRING_32} "{%"a%": [1, true, null], %"b%": %"x%"}%N")
			assert_false ("json parses", l_doc.has_error)
			assert_spans_sound ("json", l_doc)
			l_spans := l_doc.spans (1, 1)
			assert_integers_equal ("14 json spans", 14, l_spans.count)
			assert_integers_equal ("a key is a property", {TS_STYLES}.Style_variable, l_spans.style (2))
			assert_integers_equal ("true is a number", {TS_STYLES}.Style_number, l_spans.style (7))
			assert_integers_equal ("null is a constant", {TS_STYLES}.Style_constant, l_spans.style (9))
			assert_integers_equal ("a value is a string", {TS_STYLES}.Style_string, l_spans.style (13))
			assert_pair ("json brackets", l_doc.bracket_at (7), 7, 1, 21, 1)
			l_doc.set_text ({STRING_32} "{%"a%": ")
			assert_true ("half a json object is an error", l_doc.has_error)

			create l_doc.make ("c")
			l_doc.set_text ({STRING_32} "int main(void) {%N%Treturn 0; /* c */%N}%N")
			assert_false ("c parses", l_doc.has_error)
			assert_spans_sound ("c", l_doc)
			l_spans := l_doc.spans (1, 3)
			assert_integers_equal ("10 c spans", 10, l_spans.count)
			assert_integers_equal ("int is a type", {TS_STYLES}.Style_type, l_spans.style (1))
			assert_integers_equal ("main is a function", {TS_STYLES}.Style_function, l_spans.style (2))
			assert_integers_equal ("return is a keyword", {TS_STYLES}.Style_keyword, l_spans.style (7))
			assert_integers_equal ("the comment", {TS_STYLES}.Style_comment, l_spans.style (9))
			assert_integers_equal ("the body folds", 1, l_doc.folds.count)
			assert_pair ("c braces", l_doc.bracket_at (16), 16, 1, l_doc.line_start (3), 1)

			create l_doc.make ("xml")
			l_doc.set_text ({STRING_32} "<a b=%"c%">text</a>%N")
			assert_false ("xml parses", l_doc.has_error)
			assert_spans_sound ("xml", l_doc)
			l_spans := l_doc.spans (1, 1)
			assert_integers_equal ("9 xml spans", 9, l_spans.count)
			assert_integers_equal ("the element name is a tag", {TS_STYLES}.Style_tag, l_spans.style (2))
			assert_integers_equal ("the attribute value is a string", {TS_STYLES}.Style_string, l_spans.style (5))
		end

	test_fixture_files
			-- The fixture files of the C tests, read from testing/fixtures.
		local
			l_doc: TS_DOCUMENT
			l_spans: TS_SPANS
			l_string, i: INTEGER
		do
			create l_doc.make ("eiffel")
			l_doc.set_text (fixture ("sample.e"))
			assert_false ("sample.e parses clean", l_doc.has_error)
			assert_spans_sound ("sample.e", l_doc)
			assert_true ("sample.e folds", l_doc.folds.count >= 10)
				-- Line 26 is three tabs, "greeting := " and a string with an accented
				-- letter, a mathematical sign and an emoji.
			l_spans := l_doc.spans (26, 26)
			from
				i := 1
			until
				i > l_spans.count
			loop
				if l_spans.style (i) = {TS_STYLES}.Style_string then
					l_string := i
				end
				i := i + 1
			end
			assert_true ("line 26 has a string", l_string > 0)
			assert_integers_equal ("at column 16", 16, l_spans.column (l_string))
			assert_integers_equal ("17 characters long", 17, l_spans.length (l_string))

				-- The constructs the Eiffel grammar is patched for.
			create l_doc.make ("eiffel")
			l_doc.set_text (fixture ("constructs.e"))
			assert_false ("constructs.e parses clean", l_doc.has_error)
			assert_spans_sound ("constructs.e", l_doc)
			assert_true ("constructs.e folds", l_doc.folds.count >= 5)

			create l_doc.make ("c")
			l_doc.set_text (fixture ("sample.c"))
			assert_false ("sample.c parses clean", l_doc.has_error)
			assert_spans_sound ("sample.c", l_doc)

			create l_doc.make ("json")
			l_doc.set_text (fixture ("sample.json"))
			assert_false ("sample.json parses clean", l_doc.has_error)
			assert_spans_sound ("sample.json", l_doc)

			create l_doc.make ("xml")
			l_doc.set_text (fixture ("sample.ecf"))
			assert_false ("sample.ecf parses clean", l_doc.has_error)
			assert_spans_sound ("sample.ecf", l_doc)
		end

feature -- Test: memory

	test_results_free_their_blocks
			-- A result, a query, a cursor or a document that is no longer used gives
			-- its C memory back when it is collected.
		local
			l_doc: TS_DOCUMENT
			l_memory: MEMORY
			l_live, i: INTEGER
		do
			create l_memory
			l_doc := new_sample_document
			l_memory.full_collect
			l_live := l_doc.live_block_count
			from
				i := 1
			until
				i > 25
			loop
				use_and_drop (l_doc)
				i := i + 1
			end
			assert_true ("the results hold blocks until collected", l_doc.live_block_count >= l_live)
			l_memory.full_collect
			assert_true ("nothing is left once they are collected: " + l_doc.live_block_count.out
				+ " live against " + l_live.out, l_doc.live_block_count <= l_live)
		end

feature {NONE} -- Samples

	sample: STRING_32
			-- A small class, nine lines and a last line feed, with characters of one,
			-- two, three and four UTF-8 bytes. A new copy at each call.
		do
			create Result.make (160)
			Result.append ({STRING_32} "-- Caf%/0xE9/ %/0x2200/ %/0x1F600/%N")
			Result.append ({STRING_32} "class A%N")
			Result.append ({STRING_32} "feature%N")
			Result.append ({STRING_32} "%Tgreeting: STRING_32%N")
			Result.append ({STRING_32} "%T%Tdo%N")
			Result.append ({STRING_32} "%T%T%TResult := ")
			Result.append (sample_string)
			Result.append ({STRING_32} "%N")
			Result.append ({STRING_32} "%T%T%Tprint (Result [1])%N")
			Result.append ({STRING_32} "%T%Tend%N")
			Result.append ({STRING_32} "end%N")
		end

	sample_string: STRING_32
			-- The string on line 6 of `sample', with its quotes: 17 characters.
		do
			Result := {STRING_32} "%"na%/0xEF/ve %/0x2200/x %/0x1F600/ done%""
		ensure
			seventeen: Result.count = 17
		end

	new_sample_document: TS_DOCUMENT
			-- An Eiffel document holding `sample'.
		do
			create Result.make ("eiffel")
			Result.set_text (sample)
		end

	fixture (a_name: STRING): STRING_32
			-- The text of the file `a_name' in testing/fixtures, which is UTF-8. The
			-- tests run from the library's directory.
		local
			l_file: RAW_FILE
			l_bytes: STRING_8
		do
			create l_file.make_with_name ("testing/fixtures/" + a_name)
			assert_true ("testing/fixtures/" + a_name + " is there (run the tests from the library's directory)",
				l_file.exists and then l_file.is_readable)
			l_file.open_read
			l_file.read_stream (l_file.count)
			l_bytes := l_file.last_string
			l_file.close
			Result := {UTF_CONVERTER}.utf_8_string_8_to_string_32 (l_bytes)
			assert_true (a_name + " is not empty", not Result.is_empty)
		end

feature {NONE} -- Checks

	span_with_style (a_spans: TS_SPANS; a_style: INTEGER): INTEGER
			-- The number of the first span of `a_style'; 0 when none.
		local
			i: INTEGER
		do
			from
				i := 1
			until
				i > a_spans.count or Result > 0
			loop
				if a_spans.style (i) = a_style then
					Result := i
				end
				i := i + 1
			end
		end

	assert_spans_sound (a_tag: STRING; a_doc: TS_DOCUMENT)
			-- Every span of `a_doc' lies inside its line, after the span before it,
			-- and has a style.
		local
			l_spans: TS_SPANS
			l_styles: TS_STYLES
			i, l_line, l_line_length, l_last_line, l_last_end: INTEGER
		do
			create l_styles
			l_spans := a_doc.spans (1, a_doc.line_count)
			assert_false (a_tag + ": has spans", l_spans.is_empty)
			from
				i := 1
			until
				i > l_spans.count
			loop
				l_line := l_spans.line (i)
				assert_true (a_tag + ": span " + i.out + " is on a line of the text", 1 <= l_line and l_line <= a_doc.line_count)
				if l_line < a_doc.line_count then
						-- Up to, and not counting, the line feed.
					l_line_length := a_doc.line_start (l_line + 1) - 1 - a_doc.line_start (l_line)
				else
					l_line_length := a_doc.count + 1 - a_doc.line_start (l_line)
				end
				assert_true (a_tag + ": span " + i.out + " starts in its line", 1 <= l_spans.column (i))
				assert_true (a_tag + ": span " + i.out + " has a length", l_spans.length (i) >= 1)
				assert_true (a_tag + ": span " + i.out + " ends in its line",
					l_spans.column (i) + l_spans.length (i) - 1 <= l_line_length)
				assert_true (a_tag + ": span " + i.out + " comes after the one before",
					l_line > l_last_line or (l_line = l_last_line and l_spans.column (i) > l_last_end))
				assert_true (a_tag + ": span " + i.out + " has a style that is not plain",
					l_styles.is_style (l_spans.style (i)) and l_spans.style (i) /= {TS_STYLES}.Style_plain)
				l_last_line := l_line
				l_last_end := l_spans.column (i) + l_spans.length (i) - 1
				i := i + 1
			end
		end

	assert_same_as_fresh (a_tag: STRING; a_doc: TS_DOCUMENT; a_text: STRING_32)
			-- `a_doc' holds `a_text' and answers what a new document of `a_text' answers.
		local
			l_fresh: TS_DOCUMENT
			l_spans, l_fresh_spans: TS_SPANS
			l_folds, l_fresh_folds: TS_LINE_RANGES
			i: INTEGER
		do
			create l_fresh.make (a_doc.language)
			l_fresh.set_text (a_text)
			assert_true (a_tag + ": the text", a_doc.text.same_string (a_text))
			assert_integers_equal (a_tag + ": count", l_fresh.count, a_doc.count)
			assert_integers_equal (a_tag + ": line_count", l_fresh.line_count, a_doc.line_count)
			assert_true (a_tag + ": has_error", l_fresh.has_error = a_doc.has_error)
			assert_true (a_tag + ": the tree", a_doc.s_expression.same_string (l_fresh.s_expression))
			l_spans := a_doc.spans (1, a_doc.line_count)
			l_fresh_spans := l_fresh.spans (1, l_fresh.line_count)
			assert_integers_equal (a_tag + ": number of spans", l_fresh_spans.count, l_spans.count)
			from
				i := 1
			until
				i > l_spans.count
			loop
				assert_true (a_tag + ": span " + i.out,
					l_spans.line (i) = l_fresh_spans.line (i) and l_spans.column (i) = l_fresh_spans.column (i)
					and l_spans.length (i) = l_fresh_spans.length (i) and l_spans.style (i) = l_fresh_spans.style (i))
				i := i + 1
			end
			l_folds := a_doc.folds
			l_fresh_folds := l_fresh.folds
			assert_integers_equal (a_tag + ": number of folds", l_fresh_folds.count, l_folds.count)
			from
				i := 1
			until
				i > l_folds.count
			loop
				assert_true (a_tag + ": fold " + i.out,
					l_folds.first_line (i) = l_fresh_folds.first_line (i) and l_folds.last_line (i) = l_fresh_folds.last_line (i))
				i := i + 1
			end
		end

	assert_pair (a_tag: STRING; a_pair: detachable TS_BRACKET_PAIR; an_open_start, an_open_length, a_close_start, a_close_length: INTEGER)
			-- `a_pair' is there and has these ends.
		do
			if attached a_pair as al_pair then
				assert_integers_equal (a_tag + ": open start", an_open_start, al_pair.open_start)
				assert_integers_equal (a_tag + ": open length", an_open_length, al_pair.open_length)
				assert_integers_equal (a_tag + ": close start", a_close_start, al_pair.close_start)
				assert_integers_equal (a_tag + ": close length", a_close_length, al_pair.close_length)
			else
				assert_true (a_tag + ": a pair", False)
			end
		end

	use_and_drop (a_doc: TS_DOCUMENT)
			-- Make one of each thing that holds C memory, use it, and let go of it.
		local
			l_doc: TS_DOCUMENT
			l_query: TS_QUERY
			l_cursor: TS_CURSOR
			l_total: INTEGER
		do
			l_total := a_doc.spans (1, a_doc.line_count).count + a_doc.folds.count + a_doc.changed_lines.count
			create l_query.make ("eiffel", "(class_name) @name")
			l_total := l_total + a_doc.captures (l_query, 1, a_doc.count).count
			l_cursor := a_doc.new_cursor
			l_cursor.go_to_first_child
			create l_doc.make ("eiffel")
			l_doc.set_text (sample)
			l_total := l_total + l_doc.spans (1, 3).count
			assert_true ("results were made", l_total > 20)
		end

end
