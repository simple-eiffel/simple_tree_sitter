note
	description: "[
		For the tests: takes the answers out of a TS_RESULT_BOX and makes their
		readers, as TS_WORKER_CLIENT does. It is handed the blocks, so, as a TS_BLOCKS,
		it may make the readers that free them.
	]"

class
	TS_TEST_BOX_TAKER

inherit
	TS_BLOCKS

feature -- Taking

	taken_ranges (a_box: TS_RESULT_BOX): TS_LINE_RANGES
			-- The line ranges of `a_box', taken from it.
		local
			l_count: INTEGER
		do
			l_count := a_box.range_count
			create Result.make_from_block (a_box.take_ranges, l_count)
		end

	taken_spans (a_box: TS_RESULT_BOX): TS_SPANS
			-- The spans of `a_box', taken from it.
		local
			l_count: INTEGER
		do
			l_count := a_box.span_count
			create Result.make_from_block (a_box.take_spans, l_count)
		end

	taken_folds (a_box: TS_RESULT_BOX): TS_LINE_RANGES
			-- The folds of `a_box', taken from it.
		local
			l_count: INTEGER
		do
			l_count := a_box.fold_count
			create Result.make_from_block (a_box.take_folds, l_count)
		end

end
