note
	description: "[
		A text parsed in the background: the half of a TS_WORKER that stays
		with the owner of the text. It creates the worker and its box, each on a
		processor of its own, and no call here waits on a parse.

		Tell it every change (`set_text', `edit'). When results may be ready,
		which the notifier says, or a timer, call `take'. If `has_result', the
		lines of `ranges' are to be recolored from `spans', and `folds' are the
		folds; then call `confirm_applied'. A result for a text that has changed
		since is dropped inside `take', and the lines it named come again with
		the next one.

			client.edit (l_start, l_removed, l_inserted)
			...
			client.take
			if client.has_result then
				-- Recolor the lines of client.ranges from client.spans.
				client.confirm_applied
			end

		Positions count characters from 1 and lines from 1, as in TS_DOCUMENT.
	]"

class
	TS_WORKER_CLIENT

inherit
	TS_BLOCKS

create
	make

feature {NONE} -- Initialization

	make (a_language: READABLE_STRING_8; a_notifier: detachable separate TS_NOTIFIER)
			-- An empty text in `a_language'. The worker calls `a_notifier', if there
			-- is one, each time it has put something in the box.
		require
			language_known: (create {TS_LANGUAGES}).has (a_language)
		do
			create language.make_from_string (a_language)
			create box.make
			if attached a_notifier as al_notifier then
				create worker.make_with_notifier (language, box, al_notifier)
			else
				create worker.make (language, box)
			end
			create ranges.make_from_block (default_pointer, 0)
			create spans.make_from_block (default_pointer, 0)
			create folds.make_from_block (default_pointer, 0)
		ensure
			language_set: language.same_string (a_language)
			empty: count = 0
			version_zero: version = 0
			no_result: not has_result
		end

feature -- Access

	language: STRING
			-- The language of the text.

	count: INTEGER
			-- The number of characters in the text, as the changes told here leave it.

	version: INTEGER
			-- How many times the text has been set or edited.

feature -- Text

	set_text (a_text: READABLE_STRING_32)
			-- The whole text is `a_text' now.
		do
			version := version + 1
			count := a_text.count
			text_version := version
			needs_text := False
			forget_answers
			tell_wanted (box, version)
			send_text (worker, new_text_block (a_text), count, version)
		ensure
			count_set: count = a_text.count
			one_more: version = old version + 1
			no_result: not has_result
			text_given: not needs_text
		end

	edit (a_start, a_removed: INTEGER; an_inserted: READABLE_STRING_32)
			-- `an_inserted' took the place of the `a_removed' characters from position
			-- `a_start'. `a_start' = `count' + 1 with nothing removed adds at the end.
		require
			start_in_text: 1 <= a_start and a_start <= count + 1
			removed_in_text: 0 <= a_removed and a_removed <= count - a_start + 1
		do
			version := version + 1
			count := count - a_removed + an_inserted.count
			forget_answers
			tell_wanted (box, version)
			send_edit (worker, a_start, a_removed, new_text_block (an_inserted), an_inserted.count, version)
		ensure
			count_set: count = old count - a_removed + an_inserted.count
			one_more: version = old version + 1
			no_result: not has_result
		end

feature -- Results

	take
			-- Take what the box holds. A result or a bracket answer for an older text
			-- is freed and forgotten.
		do
			forget_result
			has_bracket := False
			bracket := Void
			take_from (box)
		end

	has_result: BOOLEAN
			-- Did the last `take' bring results for the text as it is now?

	ranges: TS_LINE_RANGES
			-- With `has_result': every range of lines to recolor. Empty otherwise.

	spans: TS_SPANS
			-- With `has_result': the spans of the lines of `ranges'. A line of
			-- `ranges' with no span is plain. Empty otherwise.

	folds: TS_LINE_RANGES
			-- With `has_result': the folds of the whole text. Empty otherwise.

	was_limited: BOOLEAN
			-- With `has_result': did a limit cut `spans' or `folds' short?

	confirm_applied
			-- The lines of `ranges' now show `spans': the worker need not name them
			-- again until they change.
		require
			has_result: has_result
		do
			send_applied (worker, version)
		end

	needs_text: BOOLEAN
			-- Did the worker refuse an edit, because its range was not in the worker's
			-- text? Seen by `take'. No more results come until `set_text' gives the
			-- whole text again.

feature -- Brackets

	ask_bracket (a_position: INTEGER)
			-- Ask for the bracket pair that the character at `a_position', or failing
			-- that the one before it, is an end of. The answer comes with a later `take'.
		require
			position_in_text: 1 <= a_position and a_position <= count + 1
		do
			bracket_asked := a_position
			has_bracket := False
			bracket := Void
			send_find_bracket (worker, a_position, version)
		ensure
			no_answer_yet: not has_bracket
		end

	has_bracket: BOOLEAN
			-- Did the last `take' bring the answer to the last `ask_bracket', for the
			-- text as it is now?

	bracket: detachable TS_BRACKET_PAIR
			-- With `has_bracket': the pair, or Void when there is none at the position
			-- asked. Void otherwise.

feature {NONE} -- Factory

	new_ranges (a_block: POINTER; a_count: INTEGER): TS_LINE_RANGES
			-- A reader of the `a_count' line ranges of `a_block', which it owns. A
			-- descendant may answer a descendant of TS_LINE_RANGES.
		require
			count_not_negative: a_count >= 0
			null_is_empty: a_block = default_pointer implies a_count = 0
		do
			create Result.make_from_block (a_block, a_count)
		ensure
			count_set: Result.count = a_count
		end

	new_spans (a_block: POINTER; a_count: INTEGER): TS_SPANS
			-- A reader of the `a_count' spans of `a_block', which it owns. A
			-- descendant may answer a descendant of TS_SPANS.
		require
			count_not_negative: a_count >= 0
			null_is_empty: a_block = default_pointer implies a_count = 0
		do
			create Result.make_from_block (a_block, a_count)
		ensure
			count_set: Result.count = a_count
		end

feature {NONE} -- Implementation

	worker: separate TS_WORKER
			-- The document, on its own processor.

	box: separate TS_RESULT_BOX
			-- Where the worker's answers wait, on its own processor.

	text_version: INTEGER
			-- The version the last `set_text' made.

	bracket_asked: INTEGER
			-- The position of the last `ask_bracket' since the text last changed; 0 when none.

	forget_result
			-- Let go of the result last taken, if there is one.
		do
			if has_result then
				ranges := new_ranges (default_pointer, 0)
				spans := new_spans (default_pointer, 0)
				folds := new_ranges (default_pointer, 0)
				has_result := False
			end
			was_limited := False
		ensure
			no_result: not has_result
		end

	forget_answers
			-- The text changed: what was taken is no longer for it.
		do
			forget_result
			has_bracket := False
			bracket := Void
			bracket_asked := 0
		ensure
			no_result: not has_result
			no_bracket: not has_bracket
		end

	take_from (a_box: separate TS_RESULT_BOX)
			-- Take the results and the bracket answer `a_box' holds; keep those that
			-- are for `version'.
		local
			l_version, l_position, l_range_count, l_span_count, l_fold_count: INTEGER
			l_ranges, l_spans, l_folds, l_pair: POINTER
			l_limited: BOOLEAN
		do
			if a_box.rejected_version > text_version then
				needs_text := True
			end
			if a_box.has_results then
				l_version := a_box.version
				l_limited := a_box.was_limited
				l_range_count := a_box.range_count
				l_ranges := a_box.take_ranges
				l_span_count := a_box.span_count
				l_spans := a_box.take_spans
				l_fold_count := a_box.fold_count
				l_folds := a_box.take_folds
				if l_version = version then
					ranges := new_ranges (l_ranges, l_range_count)
					spans := new_spans (l_spans, l_span_count)
					folds := new_ranges (l_folds, l_fold_count)
					was_limited := l_limited
					has_result := True
				else
					free_block (l_ranges)
					free_block (l_spans)
					free_block (l_folds)
				end
			end
			if a_box.has_bracket then
				l_version := a_box.bracket_version
				l_position := a_box.bracket_position
				l_pair := a_box.take_bracket
				if l_version = version and l_position = bracket_asked then
					has_bracket := True
					if l_pair /= default_pointer then
						bracket := new_bracket_pair (l_pair)
					end
				end
				free_block (l_pair)
			end
		end

	tell_wanted (a_box: separate TS_RESULT_BOX; a_version: INTEGER)
			-- Tell `a_box' that the change making `a_version' is on its way.
		do
			a_box.set_wanted (a_version)
		end

	send_text (a_worker: separate TS_WORKER; a_block: POINTER; a_count, a_version: INTEGER)
			-- Hand `a_worker' a whole text, and ask for its results.
		do
			a_worker.set_text (a_block, a_count, a_version)
			a_worker.refresh
		end

	send_edit (a_worker: separate TS_WORKER; a_start, a_removed: INTEGER; a_block: POINTER; a_count, a_version: INTEGER)
			-- Hand `a_worker' an edit, and ask for its results.
		do
			a_worker.edit (a_start, a_removed, a_block, a_count, a_version)
			a_worker.refresh
		end

	send_applied (a_worker: separate TS_WORKER; a_version: INTEGER)
			-- Tell `a_worker' that the results of `a_version' are shown.
		do
			a_worker.applied (a_version)
		end

	send_find_bracket (a_worker: separate TS_WORKER; a_position, a_version: INTEGER)
			-- Ask `a_worker' for the bracket pair at `a_position' of `a_version'.
		do
			a_worker.find_bracket (a_position, a_version)
		end

invariant
	count_not_negative: count >= 0
	version_not_negative: version >= 0
	text_version_not_ahead: text_version <= version
	bracket_only_with_answer: attached bracket implies has_bracket

end
