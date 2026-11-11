note
	description: "[
		Text and answers in C blocks, for handing between the processor that
		owns a document and the one that owns the text. A block is a copy:
		whoever is handed it frees it.
	]"

class
	TS_BLOCKS

inherit
	TS_API

feature -- Blocks

	new_text_block (a_text: READABLE_STRING_32): POINTER
			-- A copy of `a_text' in a C block; the null pointer when `a_text' is empty.
			-- `set_text_block' and `edit_block' of TS_DOCUMENT free the block they are
			-- given; a block handed to nothing is freed with `free_block'.
		local
			l_text: STRING_32
			l_area: ANY
		do
			if not a_text.is_empty then
				l_text := a_text.to_string_32
				l_area := l_text.to_c
				Result := c_text_new ($l_area, l_text.count)
			end
		ensure
			null_when_empty: a_text.is_empty implies Result = default_pointer
		end

	free_block (a_block: POINTER)
			-- Free a block of this library's that was handed to nothing: one made by
			-- `new_text_block', or one taken from a result or from a TS_RESULT_BOX.
			-- The null pointer is fine.
		do
			c_free (a_block)
		end

	new_bracket_pair (a_block: POINTER): TS_BRACKET_PAIR
			-- The pair in `a_block', a block from TS_DOCUMENT's `bracket_pair_block'.
			-- The block is still the caller's to free.
		require
			has_block: a_block /= default_pointer
		do
			create Result.make (c_int_at (a_block, 0) + 1, c_int_at (a_block, 1), c_int_at (a_block, 2) + 1, c_int_at (a_block, 3))
		end

	live_block_count: INTEGER
			-- C blocks and handles this library has given out and not yet freed, in the
			-- whole program. For tests.
		do
			Result := c_live_blocks
		ensure
			not_negative: Result >= 0
		end

end
