; Identifiers
(identifier) @variable

(variable
  (identifier)) @variable

(title_header
  (identifier) @type)

(jump_statement
  (identifier) @type)

; Literals
(number) @number

(string) @string

; Punctuation
[
  ":"
  "---"
  "==="
] @punctuation.delimiter

[
  "("
  ")"
  "{"
  "}"
] @punctuation.bracket

; Operators
[
  "!"
  "%"
  "&&"
  "*"
  "+"
  "-"
  "/"
  "<"
  "<="
  "="
  "=="
  ">"
  ">="
  "and"
  "eq"
  "gt"
  "gte"
  "is"
  "lt"
  "lte"
  "neq"
  "not"
  "or"
  "xor"
  "||"
] @operator

; Comments
(comment) @comment

; Keywords
[
  "if"
  "when"
] @keyword.conditional

[
  "always"
  "as"
  "call"
  "case"
  "declare"
  "detour"
  "enum"
  "jump"
  "once"
  "set"
  "title"
] @keyword

[
  "<<"
  ">>"
] @keyword.directive

; Function calls
(function_call
  function: (identifier) @function.call)

; Arrows
[
  "->"
  "=>"
] @keyword

; "Alice: Hello there!"
(text) @text
