-- Café ∀ 😀 sample for the simple_tree_sitter tests.
note
	description: "A small class with a bit of everything."

class
	SAMPLE

inherit
	ANY
		redefine
			out
		end

create
	make

feature {NONE} -- Initialization

	make (a_name: STRING; a_count: INTEGER)
			-- Set up with `a_name' and `a_count'.
		require
			name_not_empty: not a_name.is_empty
		do
			name := a_name
			count := a_count + 42
			greeting := "naïve ∀x 😀 done"
		ensure
			name_set: name = a_name
		end

feature -- Access

	name: STRING

	count: INTEGER

	greeting: STRING

	ratio: REAL_64 = 2.5

	initial: CHARACTER = 'x'

	out: STRING
		do
			Result := name + " (" + count.out + ")"
		end

feature -- Basic operations

	total (a_values: ARRAY [INTEGER]): INTEGER
			-- Sum of `a_values'.
		local
			i: INTEGER
		do
			from
				i := a_values.lower
			until
				i > a_values.upper
			loop
				if a_values [i] > 0 then
					Result := Result + a_values [i]
				elseif a_values [i] = 0 then
					count := count + 1
				else
					Result := Result - 1
				end
				i := i + 1
			end
			across a_values as v loop
				print (v.out)
			end
		end

	describe (a_flag: BOOLEAN): STRING
		do
			inspect count
			when 0 then
				Result := "none"
			when 1 .. 9 then
				Result := "few"
			else
				Result := "many"
			end
			if a_flag and then attached name as l_name then
				Result := Result + l_name
			end
			check has_result: Result /= Void end
		end

invariant
	count_not_negative: count >= 0

end
