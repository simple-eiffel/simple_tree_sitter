; JSON highlights for simple_tree_sitter.
; Adapted from tree-sitter/tree-sitter-json v0.24.8 queries/highlights.scm (MIT).

(string) @string

(pair
  key: (string) @property)

(number) @number

[
  (true)
  (false)
] @boolean

(null) @constant.builtin

[ "{" "}" "[" "]" ] @punctuation.bracket
[ "," ":" ] @punctuation.delimiter

(comment) @comment
