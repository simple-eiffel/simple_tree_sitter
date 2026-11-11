note
	description: "The two ends of a bracket pair. Positions count characters from 1."

class
	TS_BRACKET_PAIR

create
	make

feature {NONE} -- Initialization

	make (an_open_start, an_open_length, a_close_start, a_close_length: INTEGER)
		require
			open_start_positive: an_open_start >= 1
			open_length_positive: an_open_length >= 1
			close_after_open: a_close_start >= an_open_start + an_open_length
			close_length_positive: a_close_length >= 1
		do
			open_start := an_open_start
			open_length := an_open_length
			close_start := a_close_start
			close_length := a_close_length
		ensure
			open_start_set: open_start = an_open_start
			open_length_set: open_length = an_open_length
			close_start_set: close_start = a_close_start
			close_length_set: close_length = a_close_length
		end

feature -- Access

	open_start: INTEGER
			-- The position of the opening end's first character.

	open_length: INTEGER
			-- The number of characters in the opening end ("(" is 1, "do" is 2).

	close_start: INTEGER
			-- The position of the closing end's first character.

	close_length: INTEGER
			-- The number of characters in the closing end.

invariant
	open_start_positive: open_start >= 1
	open_length_positive: open_length >= 1
	close_after_open: close_start >= open_start + open_length
	close_length_positive: close_length >= 1

end
