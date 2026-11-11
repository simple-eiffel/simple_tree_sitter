; XML bracket pairs for simple_tree_sitter.

; An element's opening and closing tag names.
(element (STag (Name) @open) (ETag (Name) @close))

; The angle brackets of one tag.
(STag "<" @open ">" @close)
(ETag "</" @open ">" @close)
(EmptyElemTag "<" @open "/>" @close)
