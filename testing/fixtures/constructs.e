-- The constructs the Eiffel grammar is patched for (Clib/grammars/eiffel/simple_eiffel.patch).
class
	CONSTRUCTS

feature -- Numbers

	mask: NATURAL_32 = 0xFF_FF

	octal: INTEGER = 0c777

	bits: INTEGER = 0b1010_0101

	plain: INTEGER = 1_000

feature -- Characters

	new_line: CHARACTER = '%N'

	unit_separator: CHARACTER = '%/31/'

	smile: CHARACTER_32 = '%/0x1F600/'

	space: CHARACTER = ' '

	at: CHARACTER = '@'

	quote: CHARACTER = '%''

	double_quote: CHARACTER = '"'

	percent: CHARACTER = '%%'

	minus: CHARACTER = '-'

	accented: CHARACTER_32 = 'é'

feature -- Tuples

	cell_at (a_x, a_y: REAL_64): detachable TUPLE [row, col: INTEGER]
		do
			if a_x > 0x10 and a_y > 0 then
				Result := [1, 2]
			end
		end

	offer: detachable FUNCTION [detachable TUPLE [pebble: ANY; card: STRING_32]]

	notify (a_action: PROCEDURE)
		do
			a_action.call ([])
		end

feature -- Manifests with a type

	hash: STRING_32
		do
			Result := {STRING_32} "#"
			Result.append_character ({CHARACTER_32} ' ')
			Result.append_natural_32 ({NATURAL_32} 0x1F)
			if Result.item (1) = '#' and Result.count > 0b1 then
				Result.append_character ('%T')
			end
		end

feature -- SCOOP

	mailbox: detachable separate CONSTRUCTS

	workers: ARRAYED_LIST [separate CONSTRUCTS]

	put (a_tag: INTEGER; a_payload: separate READABLE_STRING_GENERAL)
		do
		end

end
