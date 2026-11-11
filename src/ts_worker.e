note
	description: "[
		A document on a processor of its own, so that the processor that owns
		the text (an editor's GUI processor) never waits on a parse.

		Create it separate. Its commands take numbers and pointers only: a call
		that hands over an object of the caller's would wait until the worker is
		free. Text comes in a block from TS_BLOCKS's `new_text_block', which the
		worker frees. Answers go into a TS_RESULT_BOX as blocks, which the
		client takes. TS_WORKER_CLIENT does all of this and is the class to use;
		this one is the half that runs beside it.

		A line whose highlighting changed stays owed until `applied' says the
		client has shown it. Every `refresh' answers all the lines owed, so an
		answer the client drops, or one the box replaces before the client
		looks, costs nothing.
	]"

class
	TS_WORKER

create
	make,
	make_with_notifier

feature {NONE} -- Initialization

	make (a_language: separate READABLE_STRING_8; a_box: separate TS_RESULT_BOX)
			-- A worker with an empty text in `a_language', which must be a name that
			-- TS_LANGUAGES has. It answers into `a_box'.
		do
			create language.make_from_separate (a_language)
			create document.make (language)
			box := a_box
		ensure
			version_zero: version = 0
			nothing_answered: answered_version = 0
			not_lost: not is_lost
		end

	make_with_notifier (a_language: separate READABLE_STRING_8; a_box: separate TS_RESULT_BOX; a_notifier: separate TS_NOTIFIER)
			-- As `make', and the worker calls `a_notifier' after each answer.
			--
			-- A creation procedure of its own, and not a detachable argument of `make':
			-- with EiffelStudio 25.02, creating a worker on a new processor with a void
			-- separate argument fails in that processor.
		do
			make (a_language, a_box)
			notifier := a_notifier
		ensure
			version_zero: version = 0
			nothing_answered: answered_version = 0
			not_lost: not is_lost
		end

feature -- Access

	language: STRING
			-- The language of the text.

	version: INTEGER
			-- The version of the text, as the client numbered the last change.

	answered_version: INTEGER
			-- The version `refresh' last answered for; 0 before the first answer.

feature -- Status report

	is_lost: BOOLEAN
			-- Was an edit refused because its range was not in the text? The worker
			-- no longer has the client's text then, and answers nothing until the
			-- next `set_text'.

feature -- Text

	set_text (a_block: POINTER; a_count, a_version: INTEGER)
			-- Replace the whole text with the `a_count' characters of `a_block', and
			-- call the outcome version `a_version'. Frees the block. Computes no results.
		require
			count_not_negative: a_count >= 0
			null_is_empty: a_block = default_pointer implies a_count = 0
			newer: a_version > version
		do
			document.set_text_block (a_block, a_count)
			version := a_version
			is_lost := False
		ensure
			version_set: version = a_version
			not_lost: not is_lost
		end

	edit (a_start, a_removed: INTEGER; a_block: POINTER; a_count, a_version: INTEGER)
			-- Put the `a_count' characters of `a_block' in place of the `a_removed'
			-- characters from position `a_start', and call the outcome version
			-- `a_version'. Frees the block. Computes no results.
			--
			-- The range is checked here and not in a precondition: a separate client
			-- cannot ask for the worker's count without waiting. A range that is not
			-- in the text makes the worker `is_lost' and is reported to the box.
		require
			count_not_negative: a_count >= 0
			null_is_empty: a_block = default_pointer implies a_count = 0
			newer: a_version > version
		do
			if is_lost then
				document.free_block (a_block)
			elseif
				1 <= a_start and a_start <= document.count + 1
				and 0 <= a_removed and a_removed <= document.count - a_start + 1
			then
				document.edit_block (a_start, a_removed, a_block, a_count)
			else
				document.free_block (a_block)
				is_lost := True
				put_rejected (box, a_version)
				tell_notifier
			end
			version := a_version
		ensure
			version_set: version = a_version
		end

feature -- Results

	refresh
			-- Put the results for the text as it is in the box: every line range owed,
			-- the spans of those lines, and the folds. Do nothing when the box says a
			-- newer version is on its way: the `refresh' after that change will answer.
		local
			l_ranges, l_folds: TS_LINE_RANGES
			l_spans: TS_SPANS
			l_range_count, l_span_count, l_fold_count: INTEGER
			l_limited: BOOLEAN
		do
			if not is_lost and then wanted_of (box) <= version then
				l_ranges := document.changed_lines
				l_spans := document.spans_of (l_ranges)
				l_limited := document.was_limited
				l_folds := document.folds
				l_limited := l_limited or document.was_limited
				l_range_count := l_ranges.count
				l_span_count := l_spans.count
				l_fold_count := l_folds.count
				put_results (box, l_ranges.take_block, l_range_count, l_spans.take_block, l_span_count,
					l_folds.take_block, l_fold_count, l_limited)
				answered_version := version
				tell_notifier
			end
		end

	applied (a_version: INTEGER)
			-- The client shows the results of version `a_version'. When that is the
			-- version last answered, the lines it named are no longer owed.
		do
			if a_version = answered_version then
				document.forget_changed_lines
			end
		end

	find_bracket (a_position, a_version: INTEGER)
			-- Put in the box the bracket pair at `a_position' of version `a_version',
			-- or the answer that there is none. Do nothing when the text has moved on
			-- from that version.
		do
			if
				not is_lost and then a_version = version
				and then 1 <= a_position and then a_position <= document.count + 1
			then
				put_bracket (box, a_position, document.bracket_pair_block (a_position))
				tell_notifier
			end
		end

feature {NONE} -- Implementation

	document: TS_DOCUMENT
			-- The text and its tree.

	box: separate TS_RESULT_BOX
			-- Where the answers go.

	notifier: detachable separate TS_NOTIFIER
			-- What is told after each answer.

	wanted_of (a_box: separate TS_RESULT_BOX): INTEGER
			-- The newest version the client has sent, as `a_box' knows it.
		do
			Result := a_box.wanted
		end

	put_results (a_box: separate TS_RESULT_BOX; a_ranges: POINTER; a_range_count: INTEGER; a_spans: POINTER; a_span_count: INTEGER;
			a_folds: POINTER; a_fold_count: INTEGER; a_limited: BOOLEAN)
			-- Hand the results for `version' to `a_box'.
		do
			a_box.put_results (version, a_ranges, a_range_count, a_spans, a_span_count, a_folds, a_fold_count, a_limited)
		end

	put_bracket (a_box: separate TS_RESULT_BOX; a_position: INTEGER; a_pair: POINTER)
			-- Hand the bracket answer for `a_position' in `version' to `a_box'.
		do
			a_box.put_bracket (version, a_position, a_pair)
		end

	put_rejected (a_box: separate TS_RESULT_BOX; a_version: INTEGER)
			-- Tell `a_box' that the edit making `a_version' was refused.
		do
			a_box.set_rejected (a_version)
		end

	tell_notifier
			-- Call the notifier, if there is one.
		do
			if attached notifier as al_notifier then
				notify (al_notifier)
			end
		end

	notify (a_notifier: separate TS_NOTIFIER)
			-- Tell `a_notifier' that the box holds something new.
		do
			a_notifier.notify
		end

invariant
	answered_not_ahead: answered_version <= version

end
