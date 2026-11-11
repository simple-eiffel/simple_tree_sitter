note
	description: "The languages built into the library."

class
	TS_LANGUAGES

inherit
	TS_API

feature -- Access

	names: ARRAYED_LIST [STRING]
			-- The name of each language ("eiffel", "c", "json", "xml").
		local
			i, n: INTEGER
		do
			n := c_language_count
			create Result.make (n)
			from
				i := 0
			until
				i >= n
			loop
				Result.extend (create {STRING}.make_from_c (c_language_name (i)))
				i := i + 1
			end
		ensure
			all_known: across Result as ic all has (ic) end
		end

feature -- Status report

	has (a_name: READABLE_STRING_GENERAL): BOOLEAN
			-- Is `a_name' the name of a language?
		local
			l_name: C_STRING
		do
			if not a_name.is_empty and then a_name.is_valid_as_string_8 then
				create l_name.make (a_name)
				Result := c_language_has (l_name.item)
			end
		end

	name_for_file (a_path: READABLE_STRING_GENERAL): detachable STRING
			-- The language for the file `a_path', by its extension without regard to
			-- case (".e", ".c", ".h", ".json", ".xml", ".ecf"); Void when none.
		local
			i, l_dot: INTEGER
			l_code: NATURAL_32
			l_extension: READABLE_STRING_GENERAL
			l_c: C_STRING
			l_name: POINTER
			l_done: BOOLEAN
		do
			from
				i := a_path.count
			until
				i < 1 or l_done
			loop
				l_code := a_path.code (i)
				if l_code = ('.').natural_32_code then
					l_dot := i
					l_done := True
				elseif l_code = ('/').natural_32_code or l_code = ('\').natural_32_code then
					l_done := True
				end
				i := i - 1
			end
			if l_dot > 0 and l_dot < a_path.count then
				l_extension := a_path.substring (l_dot + 1, a_path.count)
				if l_extension.is_valid_as_string_8 then
					create l_c.make (l_extension)
					l_name := c_language_for_extension (l_c.item)
					if l_name /= default_pointer then
						create Result.make_from_c (l_name)
					end
				end
			end
		ensure
			known: attached Result as al_name implies has (al_name)
		end

end
