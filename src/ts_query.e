note
	description: "[
		A tree-sitter query of the caller's own, compiled for one language. Run it
		with TS_DOCUMENT.captures. The predicates it may use are #eq?, #not-eq?,
		#any-of?, #not-any-of?, #match? and #not-match?, the last two with the
		small pattern language the library's README describes.
	]"

class
	TS_QUERY

inherit
	TS_API

	DISPOSABLE

create
	make

feature {NONE} -- Initialization

	make (a_language: READABLE_STRING_GENERAL; a_source: READABLE_STRING_GENERAL)
			-- Compile `a_source' for `a_language'. See `is_valid'.
		require
			language_known: (create {TS_LANGUAGES}).has (a_language)
		local
			l_language, l_source: C_STRING
			l_utf_8: STRING_8
			l_message: MANAGED_POINTER
			l_offset, l_kind, i, n: INTEGER
		do
			language := a_language.to_string_8
			l_utf_8 := {UTF_CONVERTER}.utf_32_string_to_utf_8_string_8 (a_source)
			create l_language.make (language)
			create l_source.make (l_utf_8)
			create l_message.make (Message_capacity)
			handle := c_query_new (l_language.item, l_source.item, l_utf_8.count, $l_offset, $l_kind,
				l_message.item, Message_capacity)
			create capture_names.make (0)
			if handle = default_pointer then
				create error_message.make_from_c (l_message.item)
				if error_message.is_empty then
					error_message := "The query could not be compiled."
				end
				error_position := character_position (l_utf_8, l_offset)
			else
				create error_message.make_empty
				pattern_count := c_query_pattern_count (handle)
				n := c_query_capture_count (handle)
				create capture_names.make (n)
				from
					i := 0
				until
					i >= n
				loop
					capture_names.extend (create {STRING}.make_from_c (c_query_capture_name (handle, i)))
					i := i + 1
				end
			end
		ensure
			language_set: language.same_string_general (a_language)
		end

feature -- Access

	language: STRING
			-- The language the query is for.

	capture_names: ARRAYED_LIST [STRING]
			-- The query's capture names, without the "@", in the order the query first
			-- uses them. Empty when not `is_valid'.

	pattern_count: INTEGER
			-- The number of patterns in the query; 0 when not `is_valid'.

	error_message: STRING
			-- What is wrong with the query; empty when `is_valid'.

	error_position: INTEGER
			-- The character of the source, from 1, where the error was found; 0 when
			-- `is_valid'.

feature -- Status report

	is_valid: BOOLEAN
			-- Did the source compile?
		do
			Result := handle /= default_pointer
		end

feature {TS_DOCUMENT} -- Implementation

	handle: POINTER
			-- The C query; null when not `is_valid'.

feature {NONE} -- Implementation

	Message_capacity: INTEGER = 256
			-- Bytes for the wrapper's error message.

	character_position (a_utf_8: STRING_8; a_byte_offset: INTEGER): INTEGER
			-- The number, from 1, of the character that the byte at `a_byte_offset'
			-- (from 0) of `a_utf_8' belongs to.
		local
			i, l_last: INTEGER
		do
			l_last := a_byte_offset.max (0).min (a_utf_8.count - 1) + 1
			from
				i := 1
			until
				i > l_last
			loop
				if a_utf_8.code (i) & 0xC0 /= 0x80 then
						-- Not a continuation byte: a character starts here.
					Result := Result + 1
				end
				i := i + 1
			end
			Result := Result.max (1)
		ensure
			positive: Result >= 1
		end

	dispose
			-- Free the C query.
		do
			if handle /= default_pointer then
				c_query_free (handle)
				handle := default_pointer
			end
		end

invariant
	message_when_invalid: is_valid = error_message.is_empty
	position_when_invalid: is_valid = (error_position = 0)
	no_names_when_invalid: not is_valid implies capture_names.is_empty and pattern_count = 0

end
