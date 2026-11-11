note
	description: "Runs the simple_tree_sitter tests and prints one line for each."

class
	TEST_APP

create
	make

feature {NONE} -- Initialization

	make
			-- Run the tests.
		do
			print ("Running simple_tree_sitter tests...%N%N")
			create lib_tests
			run_test (agent lib_tests.test_positions_count_characters, "test_positions_count_characters")
			run_test (agent lib_tests.test_languages, "test_languages")
			run_test (agent lib_tests.test_styles, "test_styles")
			run_test (agent lib_tests.test_empty_document, "test_empty_document")
			run_test (agent lib_tests.test_spans_of_a_line_range, "test_spans_of_a_line_range")
			run_test (agent lib_tests.test_folds, "test_folds")
			run_test (agent lib_tests.test_brackets, "test_brackets")
			run_test (agent lib_tests.test_node_at, "test_node_at")
			run_test (agent lib_tests.test_cursor, "test_cursor")
			run_test (agent lib_tests.test_edit_agrees_with_a_fresh_parse, "test_edit_agrees_with_a_fresh_parse")
			run_test (agent lib_tests.test_changed_lines, "test_changed_lines")
			run_test (agent lib_tests.test_text_blocks, "test_text_blocks")
			run_test (agent lib_tests.test_query, "test_query")
			run_test (agent lib_tests.test_query_errors, "test_query_errors")
			run_test (agent lib_tests.test_other_languages, "test_other_languages")
			run_test (agent lib_tests.test_fixture_files, "test_fixture_files")
			run_test (agent lib_tests.test_results_free_their_blocks, "test_results_free_their_blocks")

			print ("%N========================%N")
			print ("Results: " + passed.out + " passed, " + failed.out + " failed%N")
			if failed > 0 then
				print ("TESTS FAILED%N")
				(create {EXCEPTIONS}).die (1)
			else
				print ("ALL TESTS PASSED%N")
			end
		end

feature {NONE} -- Implementation

	lib_tests: LIB_TESTS
			-- The tests.

	passed: INTEGER
			-- Tests that passed.

	failed: INTEGER
			-- Tests that failed.

	run_test (a_test: PROCEDURE; a_name: STRING)
			-- Run `a_test' and count how it went.
		local
			l_retried: BOOLEAN
		do
			if not l_retried then
				a_test.call (Void)
				print ("  PASS: " + a_name + "%N")
				passed := passed + 1
			end
		rescue
			print ("  FAIL: " + a_name + "%N")
			print_failure_detail
			failed := failed + 1
			l_retried := True
			retry
		end

	print_failure_detail
			-- Say which assertion failed: the exception's tag and message.
		do
			if attached (create {EXCEPTION_MANAGER}).last_exception as al_exception then
				if attached al_exception.tag as al_tag then
					print ("        tag: " + al_tag.to_string_8 + "%N")
				end
				if attached al_exception.description as al_text then
					print ("        why: " + al_text.to_string_8 + "%N")
				end
			end
		end

end
