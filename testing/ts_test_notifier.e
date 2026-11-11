note
	description: "A notifier that counts how often it was told, for the worker tests. Create it separate."

class
	TS_TEST_NOTIFIER

inherit
	TS_NOTIFIER

feature -- Access

	count: INTEGER
			-- How many times `notify' was called.

feature -- Basic operations

	notify
			-- <Precursor>
		do
			count := count + 1
		ensure then
			one_more: count = old count + 1
		end

end
