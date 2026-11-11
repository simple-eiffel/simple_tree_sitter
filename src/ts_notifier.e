note
	description: "[
		Something a TS_WORKER tells when it has put an answer in its box, so
		that the client need not poll.

		The worker calls `notify' from its own processor. An object of this
		class must therefore live on a processor that is free to serve the
		call: never on one that runs an event loop.
	]"

deferred class
	TS_NOTIFIER

feature -- Basic operations

	notify
			-- The box holds something new.
		deferred
		end

end
