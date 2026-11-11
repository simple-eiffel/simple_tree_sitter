note
	description: "[
		The styles a highlight span can have. A query's capture names give them:
		`keyword', and anything under it such as `keyword.control', gives
		`Style_keyword'. The first seven are GUI_CODE_EDITOR's.
	]"

class
	TS_STYLES

inherit
	TS_API

feature -- Styles

	Style_plain: INTEGER = 0
			-- Anything no capture names.

	Style_keyword: INTEGER = 1
			-- `keyword'.

	Style_type: INTEGER = 2
			-- `type'.

	Style_string: INTEGER = 3
			-- `string' and `character'.

	Style_comment: INTEGER = 4
			-- `comment'.

	Style_number: INTEGER = 5
			-- `number' and `boolean'.

	Style_operator: INTEGER = 6
			-- `operator' and `punctuation'.

	Style_function: INTEGER = 7
			-- `function'.

	Style_constant: INTEGER = 8
			-- `constant'.

	Style_variable: INTEGER = 9
			-- `variable' and `property'.

	Style_tag: INTEGER = 10
			-- `tag' and `attribute'.

	Style_last: INTEGER = 10
			-- The highest style number.

feature -- Status report

	is_style (a_style: INTEGER): BOOLEAN
			-- Is `a_style' one of the styles?
		do
			Result := Style_plain <= a_style and a_style <= Style_last
		end

feature -- Access

	for_capture (a_name: READABLE_STRING_8): INTEGER
			-- The style the capture `a_name' gives ("keyword.control" gives
			-- `Style_keyword'); `Style_plain' for a name that gives none.
		local
			l_name: C_STRING
		do
			create l_name.make (a_name)
			Result := c_style_for_capture (l_name.item)
		ensure
			is_style: is_style (Result)
		end

	name (a_style: INTEGER): STRING
			-- A word for `a_style' ("keyword"), for tools and test output.
		require
			is_style: is_style (a_style)
		do
			inspect a_style
			when Style_keyword then Result := "keyword"
			when Style_type then Result := "type"
			when Style_string then Result := "string"
			when Style_comment then Result := "comment"
			when Style_number then Result := "number"
			when Style_operator then Result := "operator"
			when Style_function then Result := "function"
			when Style_constant then Result := "constant"
			when Style_variable then Result := "variable"
			when Style_tag then Result := "tag"
			else
				Result := "plain"
			end
		ensure
			round_trip: for_capture (Result) = a_style
		end

end
