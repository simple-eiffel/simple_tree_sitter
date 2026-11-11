note
	description: "[
		What a syntax node is and where: a plain value, good after the document
		changes (it then describes the text as it was). Positions and lines count
		from 1.
	]"

class
	TS_NODE_INFO

create
	make

create {TS_DOCUMENT, TS_CURSOR}
	make_from_c

feature {NONE} -- Initialization

	make_from_c (a_type: POINTER; an_info: MANAGED_POINTER)
			-- From what the wrapper answered: the type as a C string and seven integers
			-- (start, length, first line, last line, all from 0; named, error, missing).
		require
			type_given: a_type /= default_pointer
			seven_integers: an_info.count >= 28
		do
			make (create {STRING}.make_from_c (a_type),
				an_info.read_integer_32 (0) + 1, an_info.read_integer_32 (4),
				an_info.read_integer_32 (8) + 1, an_info.read_integer_32 (12) + 1,
				an_info.read_integer_32 (16) /= 0, an_info.read_integer_32 (20) /= 0,
				an_info.read_integer_32 (24) /= 0)
		end

	make (a_type: STRING; a_start, a_length, a_first_line, a_last_line: INTEGER; a_named, an_error, a_missing: BOOLEAN)
		require
			start_positive: a_start >= 1
			length_not_negative: a_length >= 0
			first_line_positive: a_first_line >= 1
			last_line_not_before: a_last_line >= a_first_line
		do
			type := a_type
			start := a_start
			length := a_length
			first_line := a_first_line
			last_line := a_last_line
			is_named := a_named
			is_error := an_error
			is_missing := a_missing
		ensure
			type_set: type = a_type
			start_set: start = a_start
			length_set: length = a_length
			first_line_set: first_line = a_first_line
			last_line_set: last_line = a_last_line
			named_set: is_named = a_named
			error_set: is_error = an_error
			missing_set: is_missing = a_missing
		end

feature -- Access

	type: STRING
			-- The node's type in the grammar ("class_declaration", "identifier", ";").

	start: INTEGER
			-- The position of the node's first character.

	length: INTEGER
			-- The number of characters the node covers.

	first_line: INTEGER
			-- The line the node starts on.

	last_line: INTEGER
			-- The line the node's last character is on.

feature -- Status report

	is_named: BOOLEAN
			-- Is this a named node of the grammar, not a piece of punctuation or a keyword?

	is_error: BOOLEAN
			-- Is this an error node, text the parser could not place?

	is_missing: BOOLEAN
			-- Did the parser put this node in to recover from text that is not there?

invariant
	start_positive: start >= 1
	length_not_negative: length >= 0
	first_line_positive: first_line >= 1
	last_line_not_before: last_line >= first_line

end
