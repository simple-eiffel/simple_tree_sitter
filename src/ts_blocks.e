note
	description: "[
		Text in C blocks, for handing to the processor that owns a document.
		A block is a copy: whoever is handed it frees it.
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
			-- Free a block made by `new_text_block' and handed to nothing.
		do
			c_free (a_block)
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
