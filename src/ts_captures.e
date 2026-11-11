note
	description: "[
		What one run of a TS_QUERY captured in a document, in the order of the
		text. Positions count characters from 1.
	]"

class
	TS_CAPTURES

inherit
	TS_INT_BLOCK

create {TS_BLOCKS}
	make
			-- The object frees the block it is made from, so only a class that is handed
			-- blocks (TS_BLOCKS: TS_DOCUMENT, and the worker's client, which takes them
			-- from the box) may make one.

feature {NONE} -- Initialization

	make (a_block: POINTER; a_count: INTEGER; a_names: ARRAYED_LIST [STRING])
			-- Take over `a_block' (see `make_from_block'); `a_names' are the capture
			-- names of the query that was run.
		require
			count_not_negative: a_count >= 0
			null_is_empty: a_block = default_pointer implies a_count = 0
		do
			names := a_names
			make_from_block (a_block, a_count)
		ensure
			names_set: names = a_names
			count_set: count = a_count
		end

feature -- Access

	item_size: INTEGER = 4
			-- Integers for each capture: pattern, capture, start, length.

	names: ARRAYED_LIST [STRING]
			-- The query's capture names, without the "@".

	pattern (i: INTEGER): INTEGER
			-- The number, from 1, of the pattern in the query that matched.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 0) + 1
		ensure
			positive: Result >= 1
		end

	capture (i: INTEGER): INTEGER
			-- The number in `names' of capture `i''s name.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 1) + 1
		ensure
			positive: Result >= 1
		end

	name (i: INTEGER): STRING
			-- The name of capture `i'.
		require
			valid_index: valid_index (i)
			name_known: names.valid_index (capture (i))
		do
			Result := names.i_th (capture (i))
		end

	start (i: INTEGER): INTEGER
			-- The position of the first character capture `i' covers.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 2) + 1
		ensure
			positive: Result >= 1
		end

	length (i: INTEGER): INTEGER
			-- The number of characters capture `i' covers.
		require
			valid_index: valid_index (i)
		do
			Result := value (i, 3)
		ensure
			not_negative: Result >= 0
		end

end
