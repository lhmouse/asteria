# Asteria Grammar

## Lexicology

* _identifier_ ::=
  - `[A-Za-z_][A-Za-z_0-9]*`

* _numeric-literal_ ::=
  - `[-+]?nan`
  - `[-+]?NaN`
  - `[-+]?infinity`
  - `[-+]?Infinity`
  - ``[-+]?0[bB][01`]+(\.[01`]+)?([pP][-+]?[0-9`]+)?``
  - ``[-+]?[0-9][0-9`]*(\.[0-9`]+)?([eE][-+]?[0-9`]+)?``
  - ``[-+]?0[xX][0-9a-fA-F`]+(\.[0-9a-fA-F`]+)?([pP][-+]?[0-9`]+)?``

* _string-literal_ ::=
  - `"([^"\\]|\\([abfnrtveZ0'"?\\/]|x[0-9A-Fa-f]{2}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{6}))*"`
  - `'[^']*'`

## Syntax

* _document_ ::=
  - ( _statement_ )\*

* _statement_ ::=
  - _variable-definition_
  - _immutable-variable-definition_
  - _reference-definition_
  - _function-definition_
  - _defer-statement_
  - _null-statement_
  - _nondeclarative-statement_

* _variable-definition_ ::=
  - `var` _variable-declarator_ ( _equal-initializer_ )? ( `,` _variable-declarator_
    ( _equal-initializer_ )? )\* `;`

* _variable-declarator_ ::=
  - _identifier_
  - `[` _identifier-list_ `]`
  - `{` _identifier-list_ `}`

* _identifier-list_ ::=
  - _identifier_ ( `,` _identifier_ )\* ( `,` )?

* _equal-initializer_ ::=
  - `=` _expression_

* _immutable-variable-definition_ ::=
  - `const` _variable-declarator_ _equal-initializer_ ( `,` _variable-declarator_
    _equal-initializer_ )\* `;`

* _reference-definition_ ::=
  - `ref` _identifier_ _arrow-initializer_ ( `,` _identifier_ _arrow-initializer_ )\* `;`

* _arrow-initializer_ ::=
  - `->` _expression_

* _function-definition_ ::=
  - `func` _identifier_ `(` ( _parameter-list_ )? `)` _statement-block_

* _parameter-list_ ::=
  - `...`
  - _identifier_ ( `,` _identifier_ )\* ( `,` ( `...` )? )?

* _defer-statement_ ::=
  - `defer` _expression_ `;`

* _null-statement_ ::=
  - `;`

* _nondeclarative-statement_ ::=
  - _if-statement_
  - _switch-statement_
  - _do-while-statement_
  - _while-statement_
  - _for-statement_
  - _break-statement_
  - _continue-statement_
  - _throw-statement_
  - _return-statement_
  - _assert-statement_
  - _try-statement_
  - _statement-block_
  - _expression-statement_

* `if-statement` ::=
  - `if` ( _negation_ )? `(` _expression_ `)` _nondeclarative-statement_ ( `else`
    _nondeclarative-statement_ )?

* negation ::=
  - `!`
  - `not`

* _switch-statement_ ::=
  - `switch` `(` _expression_ `)` `{` ( _switch-clause_ )\* `}`

* _switch-clause_ ::=
  - `default` `:` ( _statement_ )\*
  - `case` _expression_ `:` ( _statement_ )\*
  - `each` ( `[` | `(` ) _expression_ `,` _expression_ ( `]` | `)` ) `:` ( _statement_ )\*

* _do-while-statement_ ::=
  - `do` _nondeclarative-statement_ `while` ( _negation_ )? `(` _expression_ `)` `;`

* _while-statement_ ::=
  - `while` ( _negation_ )? `(` _expression_ `)` _nondeclarative-statement_

* _for-statement_ ::=
  - `for` `(` _for-complement_

* _for-complement_ ::=
  - _for-complement-range_
  - _for-complement-triplet_

* _for-complement-range_ ::=
  - `each` _identifier_ ( ( `,` | `:` | `=` ) _identifier_ )? _arrow-initializer_ `)`
    _nondeclarative-statement_

* _for-complement-triplet_ ::=
  - _for-initializer_ ( _expression_ )? `;` ( _expression_ )? `)`
    _nondeclarative-statement_

* _for-initializer_ ::=
  - _variable-definition_
  - _immutable-variable-definition_
  - _reference-definition_
  - _null-statement_
  - _expression-statement_

* _break-statement_ ::=
  - `break` ( `switch` | `while` | `for` )? `;`

* _continue-statement_ ::=
  - `continue` ( `while` | `for` )? `;`

* _throw-statement_ ::=
  - `throw` _expression_ `;`

* _return-statement_ ::=
  - `return` ( _argument_ )? `;`

* _argument_ ::=
  - ( `ref` | `->` )? _expression_

* _assert-statement_ ::=
  - `assert` _expression_ ( `:` _string-literal_ )? `;`

* _try-statement_ ::=
  - `try` _nondeclarative-statement_ `catch` `(` _identifier_ `)`
    _nondeclarative-statement_

* _statement-block_ ::=
  - `{` ( statement )\* `}`

* _expression-statement_ ::=
  - _expression_ `;`

* _expression_ ::=
  - _infix-element_ ( _infix-operator_ _infix-element_ )\*

* _infix-element_ ::=
  - ( _prefix-operator_ )\* _primary-expression_ ( _postfix-operator_ )\*

* _prefix-operator_ ::=
  - `+`
  - `-`
  - `~`
  - `!`
  - `++`
  - `--`
  - `#`
  - `unset`
  - `countof`
  - `typeof`
  - `not`
  - `__abs`
  - `__sqrt`
  - `__sign`
  - `__isnan`
  - `__isinf`
  - `__round`
  - `__floor`
  - `__ceil`
  - `__trunc`
  - `__iround`
  - `__ifloor`
  - `__iceil`
  - `__itrunc`
  - `__lzcnt`
  - `__tzcnt`
  - `__popcnt`
  - `__isvoid`

* _primary-expression_ ::=
  - _literal_
  - _identifier_
  - _extern-identifier_
  - `this`
  - _closure-function_
  - _unnamed-array_
  - _unnamed-object_
  - _nested-expression_
  - _fused-multiply-add_
  - _prefix-binary-expression_
  - _catch-expression_
  - _variadic-function-call_
  - _import-function-call_

* _literal_ ::=
  - `null`
  - `false`
  - `true`
  - _numeric-literal_
  - ( _string-literal_ )\*

* _extern-identifier_ ::=
  - `extern` _identifier_

* _closure-function_ ::=
  - `func` `(` ( _parameter-list_ )? `)` _closure-body_

* _closure-body_ ::=
  - _equal-initializer_
  - _arrow-initializer_
  - _statement-block_

* _unnamed-array_ ::=
  - `[` ( _unnamed-array-element-list_ )? `]`

* _unnamed-array-element-list_ ::=
  - _expression_ ( ( `,` | `;` ) _expression_ )* ( `,` | `;` )?

* _unnamed-object_ ::=
  - `{` ( _unnamed-object-element-list_ )? `}`

* _unnamed-object-element-list_ ::=
  - _json5-key_ ( `=` | `:` ) _expression_ ( ( `,` | `;` ) _json5-key_
    ( `=` | `:` ) _expression_ )* ( `,` | `;` )?

* _json5-key_ ::=
  - _string-literal_
  - _identifier_

* _nested-expression_ ::=
  - `(` _expression_ `)`

* _fused-multiply-add_ ::=
  - `__fma` `(` _expression_ `,` _expression_ `,` _expression_ `)`

* _prefix-binary-expression_ ::=
  - _prefix-binary-operator_ `(` _expression_ `,` _expression_ `)`

* _prefix-binary-operator_ ::=
  - `__addm`
  - `__subm`
  - `__mulm`
  - `__adds`
  - `__subs`
  - `__muls`

* _catch-expression_ ::=
  - `catch` `(` _expression_ `)`

* _variadic-function-call_ ::=
  - `__vcall` `(` _expression_ `,` _expression_ `)`

* _import-function-call_ ::=
  - `import` `(` _argument-list_ `)`

* _argument-list_ ::=
  - _argument_ ( `,` _argument_ )? ( `,` )?

* _postfix-operator_ ::=
  - `++`
  - `--`
  - `[^]`
  - `[$]`
  - `[?]`
  - _postfix-function-call_
  - _postfix-subscript_
  - _postfix-member-access_
  - _postfix-operator_

* _postfix-function-call_ ::=
  - `(` ( _argument-list_ )? `)`

* _postfix-subscript_ ::=
  - `[` _expression_ `]`

* _postfix-member-access_ ::=
  - `.` json5-key

* _infix-operator_ ::=
  - _infix-ternary_
  - _infix-logical-and_
  - _infix-logical-or_
  - _infix-coalescence_
  - _infix-operator-general_

* _infix-ternary_ ::=
  - `?` _expression_ `:`
  - `?=` _expression_ `:`

* _infix-logical-and_ ::=
  - `&&`
  - `&&=`
  - `and`

* _infix-logical-or_ ::=
  - `||`
  - `||=`
  - `or`

* _infix-coalescence_ ::=
  - `??`
  - `??=`

* _infix-general_ ::=
  - `+`
  - `-`
  - `*`
  - `/`
  - `%`
  - `<<`
  - `>>`
  - `<<<`
  - `>>>`
  - `&`
  - `|`
  - `^`
  - `+=`
  - `-=`
  - `*=`
  - `/=`
  - `%=`
  - `<<=`
  - `>>=`
  - `<<<=`
  - `>>>=`
  - `&=`
  - `|=`
  - `^=`
  - `=`
  - `==`
  - `!=`
  - `<`
  - `>`
  - `<=`
  - `>=`
  - `<=>`
  - `</>`

## Operator Precedence

> [!IMPORTANT]
> Bitwise, equality and coalescence operators have different precedence from
> other languages. The consideration is based on estimation about their usual
> use cases: More common cases deserve higher precedence and fewer parentheses.

|Precedence (descending)   |Operators                     |Associativity |
|:-------------------------|:-----------------------------|:-------------|
|Postfix operators         |`++` `--` `( )` `[ ]` ...     |Left-to-right |
|Prefix operators          |`++` `--` `!` `countof` ...   |Right-to-left |
|Coalescence operator      |`??`                          |Left-to-right |
|Multiplicative operators  |`*` `/` `%`                   |Left-to-right |
|Additive operators        |`+` `-`                       |Left-to-right |
|Shift operators           |`<<<` `>>>` `<<` `>>`         |Left-to-right |
|Bitwise AND operator      |`&`                           |Left-to-right |
|Bitwise OR operators      |`\|` `^`                      |Left-to-right |
|Order operators           |`<` `>` `<=` `>=`             |Left-to-right |
|Equality operators        |`==` `!=` `<=>` `</>`         |Left-to-right |
|Logical AND operator      |`&&`                          |Left-to-right |
|Logical OR operator       |`\|\|`                        |Left-to-right |
|Conditional & Assignment  |`? :` `=` `+=` `&&=` ...      |Right-to-left |
|Special operators         |`catch` `__fma` `__vcall` ... |N/A           |
