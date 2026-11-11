note
	description: "[
		A block of integers the C wrapper answered, a fixed number of them for
		each item. The object owns the block and frees it when collected.
	]"

deferred class
	TS_INT_BLOCK

inherit
	TS_API

	DISPOSABLE

feature {NONE} -- Initialization

	make_from_block (a_block: POINTER; a_count: INTEGER)
			-- Take over `a_block', which the wrapper allocated and which holds `a_count'
			-- items of `item_size' integers each. The null pointer with 0 is the empty result.
		require
			count_not_negative: a_count >= 0
			null_is_empty: a_block = default_pointer implies a_count = 0
		do
			block := a_block
			count := a_count
		ensure
			count_set: count = a_count
		end

feature -- Access

	count: INTEGER
			-- Number of items.

	item_size: INTEGER
			-- Integers for each item.
		deferred
		ensure
			positive: Result > 0
		end

feature -- Status report

	is_empty: BOOLEAN
			-- Are there no items?
		do
			Result := count = 0
		ensure
			definition: Result = (count = 0)
		end

	valid_index (i: INTEGER): BOOLEAN
			-- Is `i' the number of an item?
		do
			Result := 1 <= i and i <= count
		ensure
			definition: Result = (1 <= i and i <= count)
		end

feature {NONE} -- Implementation

	block: POINTER
			-- The C block; null when empty.

	value (i, a_field: INTEGER): INTEGER
			-- Integer `a_field' (from 0) of item `i', as the wrapper wrote it.
		require
			valid_index: valid_index (i)
			valid_field: 0 <= a_field and a_field < item_size
		do
			Result := c_int_at (block, (i - 1) * item_size + a_field)
		end

	dispose
			-- Free the C block.
		do
			if block /= default_pointer then
				c_free (block)
				block := default_pointer
			end
		end

invariant
	count_not_negative: count >= 0
	empty_without_block: block = default_pointer implies count = 0

end
