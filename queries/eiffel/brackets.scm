; Eiffel bracket pairs for simple_tree_sitter: @open and @close are the two ends.

(_ "(" @open ")" @close)
(_ "[" @open "]" @close)
(_ "{" @open "}" @close)
(_ "<<" @open ">>" @close)

; Blocks and the `end' that closes them.
(class_declaration "class" @open "end" @close)
(conditional "if" @open "end" @close)
(multi_branch "inspect" @open "end" @close)
(check "check" @open "end" @close)
(debug "debug" @open "end" @close)
(attribute_or_routine (feature_body (effective_routine (internal (routine_mark "do" @open)))) "end" @close)
(attribute_or_routine (feature_body (effective_routine (internal (routine_mark (once "once" @open))))) "end" @close)
(attribute_or_routine (feature_body (effective_routine (external "external" @open))) "end" @close)
(attribute_or_routine (feature_body (attribute "attribute" @open)) "end" @close)
(attribute_or_routine (feature_body "deferred" @open) "end" @close)
(loop (loop_body "loop" @open) "end" @close)
