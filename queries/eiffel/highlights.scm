; Eiffel highlights for simple_tree_sitter.
; Written for the grammar imustafin/tree-sitter-eiffel v1.0.0 (MIT) with
; Clib/grammars/eiffel/simple_eiffel.patch.
; A later pattern wins over an earlier one on the same text.

(identifier) @variable

[
  "across" "agent" "alias" "all" "and" "and then" "as" "assign" "attached"
  "attribute" "check" "class" "convert" "create" "debug" "deferred"
  "detachable" "do" "else" "elseif" "end" "ensure" "expanded" "export"
  "external" "feature" "from" "frozen" "if" "implies" "inherit" "inspect"
  "invariant" "like" "local" "loop" "note" "obsolete" "old" "once" "or"
  "or else" "redefine" "rename" "require" "rescue" "select" "separate" "some"
  "then"
  "undefine" "until" "variant" "when" "xor" "Precursor"
] @keyword

[
  (current)
  (result)
  (void)
  (retry)
  (unary_not)
] @keyword

(class_name) @type
"NONE" @type

(boolean_constant) @boolean
(integer_constant) @number
(real_constant) @number
(character_constant) @character

(basic_manifest_string) @string
(verbatim_string) @string

[
  ":=" "=" "/=" "~" "/~" "<" ">" "<=" ">=" "+" "-" "*" "/" "//" "\\\\" "^"
  ".." "->" "?=" "@" "$"
] @operator
(free_operator) @operator

[ "(" ")" "[" "]" "{" "}" "<<" ">>" ] @punctuation.bracket
[ "," ";" ":" "." ] @punctuation.delimiter

; The point and the sign of a number are part of the number.
(real "." @number)
(sign [ "+" "-" ] @number)

(extended_feature_name (identifier) @function)

(tag) @label

[
  (comment)
  (header_comment)
] @comment
