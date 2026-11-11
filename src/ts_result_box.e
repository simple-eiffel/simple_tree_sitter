note
	description: "[
		What a TS_WORKER last answered, kept where the worker and its client
		both reach it without waiting on each other: on a processor of its own,
		which only stores and hands over.

		It also holds the newest version of the text the client has sent
		(`wanted'), so that the worker can skip results nobody will use.

		Answers are C blocks, handed in and out as a pointer and a count, so no
		Eiffel object crosses between processors. The box owns a block from
		`put_results' or `put_bracket' until it is taken. It frees what it
		replaces, and what it still holds when it is collected.
	]"

class
	TS_RESULT_BOX

inherit
	TS_API

	DISPOSABLE

create
	make

feature {NONE} -- Initialization

	make
			-- An empty box.
		do
		ensure
			no_results: not has_results
			no_bracket: not has_bracket
		end

feature -- The version wanted

	wanted: INTEGER
			-- The newest version of the text the client has sent to the worker.

	set_wanted (a_version: INTEGER)
			-- The client is sending the change that makes version `a_version'.
		require
			not_older: a_version >= wanted
		do
			wanted := a_version
		ensure
			wanted_set: wanted = a_version
		end

feature -- Results

	has_results: BOOLEAN
			-- Are there results that nobody has taken from yet?

	version: INTEGER
			-- The version of the text the newest results are for; 0 before the first.

	was_limited: BOOLEAN
			-- Did a limit cut the newest results short?

	range_count: INTEGER
			-- The number of line ranges in the block `take_ranges' hands on.

	span_count: INTEGER
			-- The number of spans in the block `take_spans' hands on.

	fold_count: INTEGER
			-- The number of folds in the block `take_folds' hands on.

	put_results (a_version: INTEGER; a_ranges: POINTER; a_range_count: INTEGER; a_spans: POINTER; a_span_count: INTEGER;
			a_folds: POINTER; a_fold_count: INTEGER; a_limited: BOOLEAN)
			-- Keep the results for version `a_version': the line ranges that were owed,
			-- the spans of those lines, and the folds, each a block as TS_LINE_RANGES
			-- and TS_SPANS read them. The blocks are the box's from now on. Results
			-- that were not taken are freed.
		require
			range_count_not_negative: a_range_count >= 0
			no_ranges_without_block: a_ranges = default_pointer implies a_range_count = 0
			span_count_not_negative: a_span_count >= 0
			no_spans_without_block: a_spans = default_pointer implies a_span_count = 0
			fold_count_not_negative: a_fold_count >= 0
			no_folds_without_block: a_folds = default_pointer implies a_fold_count = 0
		do
			free_results
			version := a_version
			ranges := a_ranges
			range_count := a_range_count
			spans := a_spans
			span_count := a_span_count
			folds := a_folds
			fold_count := a_fold_count
			was_limited := a_limited
			has_results := True
		ensure
			has_results: has_results
			version_set: version = a_version
			range_count_set: range_count = a_range_count
			span_count_set: span_count = a_span_count
			fold_count_set: fold_count = a_fold_count
			limited_set: was_limited = a_limited
		end

	take_ranges: POINTER
			-- The block of line ranges, `range_count' of them: read the count first.
			-- The block is the caller's from now on, and the box forgets it.
		do
			Result := ranges
			ranges := default_pointer
			range_count := 0
			has_results := False
		ensure
			forgotten: range_count = 0
			taken_from: not has_results
		end

	take_spans: POINTER
			-- The block of spans, `span_count' of them: read the count first.
			-- The block is the caller's from now on, and the box forgets it.
		do
			Result := spans
			spans := default_pointer
			span_count := 0
			has_results := False
		ensure
			forgotten: span_count = 0
			taken_from: not has_results
		end

	take_folds: POINTER
			-- The block of folds, `fold_count' of them: read the count first.
			-- The block is the caller's from now on, and the box forgets it.
		do
			Result := folds
			folds := default_pointer
			fold_count := 0
			has_results := False
		ensure
			forgotten: fold_count = 0
			taken_from: not has_results
		end

feature -- The bracket answer

	has_bracket: BOOLEAN
			-- Is there a bracket answer that nobody has taken yet?

	bracket_version: INTEGER
			-- The version of the text the bracket answer is for.

	bracket_position: INTEGER
			-- The position the bracket answer is for.

	put_bracket (a_version, a_position: INTEGER; a_pair: POINTER)
			-- Keep the answer for `a_position' in version `a_version': `a_pair' is a
			-- block from TS_DOCUMENT's `bracket_pair_block', the box's from now on, or
			-- the null pointer when there is no pair there. An answer that was not
			-- taken is freed.
		do
			free_bracket
			bracket_version := a_version
			bracket_position := a_position
			bracket := a_pair
			has_bracket := True
		ensure
			has_bracket: has_bracket
			version_set: bracket_version = a_version
			position_set: bracket_position = a_position
		end

	take_bracket: POINTER
			-- The block of the bracket answer; the null pointer when the answer is
			-- that there is no pair. The block is the caller's from now on, and the
			-- box forgets the answer.
		do
			Result := bracket
			bracket := default_pointer
			has_bracket := False
		ensure
			taken: not has_bracket
		end

feature -- Refused edits

	rejected_version: INTEGER
			-- The newest version whose edit the worker refused, because its range was
			-- not in the worker's text; 0 when none was. The worker answers nothing
			-- more until it is given the whole text again.

	set_rejected (a_version: INTEGER)
			-- The worker refused the edit that makes version `a_version'.
		do
			rejected_version := rejected_version.max (a_version)
		ensure
			not_less: rejected_version >= a_version
		end

feature -- Removal

	discard
			-- Free every block held.
		do
			free_results
			free_bracket
		ensure
			no_results: not has_results
			no_bracket: not has_bracket
		end

feature {NONE} -- Implementation

	ranges: POINTER
			-- The block of owed line ranges; null when none or taken.

	spans: POINTER
			-- The block of spans; null when none or taken.

	folds: POINTER
			-- The block of folds; null when none or taken.

	bracket: POINTER
			-- The block of the bracket pair; null when none or taken.

	free_results
			-- Free the result blocks still held.
		do
			c_free (ranges)
			ranges := default_pointer
			range_count := 0
			c_free (spans)
			spans := default_pointer
			span_count := 0
			c_free (folds)
			folds := default_pointer
			fold_count := 0
			has_results := False
		end

	free_bracket
			-- Free the bracket block if it is still held.
		do
			c_free (bracket)
			bracket := default_pointer
			has_bracket := False
		end

	dispose
			-- Free every block held.
		do
			free_results
			free_bracket
		end

invariant
	no_ranges_without_block: ranges = default_pointer implies range_count = 0
	no_spans_without_block: spans = default_pointer implies span_count = 0
	no_folds_without_block: folds = default_pointer implies fold_count = 0
	counts_not_negative: range_count >= 0 and span_count >= 0 and fold_count >= 0

end
