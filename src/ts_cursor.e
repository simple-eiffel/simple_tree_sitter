note
	description: "[
		A walk over a document's syntax tree, one node at a time, starting at the
		root. Made by TS_DOCUMENT.new_cursor. It is good for the document `version'
		it was made at: once the document changes, make a new one.
	]"

class
	TS_CURSOR

inherit
	TS_API

	DISPOSABLE

create {TS_DOCUMENT}
	make

feature {NONE} -- Initialization

	make (a_document: TS_DOCUMENT; a_handle: POINTER)
			-- A cursor on the root of `a_document''s tree, over the C cursor `a_handle'.
		do
			document := a_document
			version := a_document.version
			handle := a_handle
		ensure
			document_set: document = a_document
			version_set: version = a_document.version
		end

feature -- Access

	document: TS_DOCUMENT
			-- The document whose tree this walks.

	version: INTEGER
			-- The document's `version' when the cursor was made.

	node: TS_NODE_INFO
			-- The node the cursor is on.
		require
			valid: is_valid
		local
			l_info: MANAGED_POINTER
			l_type: POINTER
		do
			create l_info.make (28)
			l_type := c_cursor_node (handle, l_info.item)
			check on_a_node: l_type /= default_pointer then
				create Result.make_from_c (l_type, l_info)
			end
		end

feature -- Status report

	is_valid: BOOLEAN
			-- Is the document as it was when the cursor was made?
		do
			Result := handle /= default_pointer and document.version = version
		end

	has_moved: BOOLEAN
			-- Did the last move find a node to go to? When not, the cursor stayed.

feature -- Cursor movement

	go_to_first_child
			-- Move to the first child of `node'; see `has_moved'.
		require
			valid: is_valid
		do
			has_moved := c_cursor_goto_first_child (handle)
		end

	go_to_next_sibling
			-- Move to the node after `node' under the same parent; see `has_moved'.
		require
			valid: is_valid
		do
			has_moved := c_cursor_goto_next_sibling (handle)
		end

	go_to_parent
			-- Move to the parent of `node'; see `has_moved'.
		require
			valid: is_valid
		do
			has_moved := c_cursor_goto_parent (handle)
		end

feature {NONE} -- Implementation

	handle: POINTER
			-- The C cursor.

	dispose
			-- Free the C cursor.
		do
			if handle /= default_pointer then
				c_cursor_free (handle)
				handle := default_pointer
			end
		end

end
