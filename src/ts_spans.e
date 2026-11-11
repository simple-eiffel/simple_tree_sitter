note
	description: "[
		Highlight spans for a range of lines, as TS_DOCUMENT.spans answers them:
		in order, never overlapping, never across a line end. Lines, and columns
		within a line, count characters from 1. Text between spans is plain.
	]"

class
	TS_SPANS

inherit
	TS_INT_BLOCK

create {TS_BLOCKS}
	make_from_block
			-- The object frees the block it is made from, so only a class that is handed
			-- blocks (TS_BLOCKS: TS_DOCUMENT, and the worker's client, which takes them
			-- from the box) may make one.

feature -- Access

	item_size: INTEGER = 4
			-- Integers for each span: line, column, length, style.

	line (i: INTEGER): INTEGER
			-- The line span `i' is on.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 0) + 1
		ensure
			positive: Result >= 1
		end

	column (i: INTEGER): INTEGER
			-- The column of span `i''s first character.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 1) + 1
		ensure
			positive: Result >= 1
		end

	length (i: INTEGER): INTEGER
			-- The number of characters in span `i'.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 2)
		ensure
			positive: Result >= 1
		end

	style (i: INTEGER): INTEGER
			-- The style of span `i', one of TS_STYLES.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 3)
		end

end
