note
	description: "[
		Ranges of lines, in order: the folds of a document, or the lines whose
		highlighting it owes. Lines count from 1.
	]"

class
	TS_LINE_RANGES

inherit
	TS_INT_BLOCK

create {TS_DOCUMENT}
	make_from_block
			-- The object frees the block it is made from, so only the class that got the
			-- block from the wrapper may make one.

feature -- Access

	item_size: INTEGER = 2
			-- Integers for each range: first line, last line.

	first_line (i: INTEGER): INTEGER
			-- The first line of range `i'.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 0) + 1
		ensure
			positive: Result >= 1
		end

	last_line (i: INTEGER): INTEGER
			-- The last line of range `i'.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 1) + 1
		ensure
			not_before_first: Result >= first_line (i)
		end

feature -- Status report

	has_line (a_line: INTEGER): BOOLEAN
			-- Is `a_line' in one of the ranges?
		local
			i: INTEGER
		do
			from
				i := 1
			until
				i > count or Result
			loop
				Result := first_line (i) <= a_line and a_line <= last_line (i)
				i := i + 1
			end
		end

end
