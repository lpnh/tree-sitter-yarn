/**
 * @file Yarn grammar for tree-sitter
 * @author lpnh <paniguel.lpnh@gmail.com>
 * @license Unlicense
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: 'yarn_spinner',

  conflicts: $ => [[$.line_group_statement]],

  externals: $ => [$._indent, $._dedent],

  extras: $ => [/\s/, $.comment],

  word: $ => $.identifier,

  rules: {
    dialogue: $ =>
      seq(repeat(alias($.hashtag, $.file_hashtag)), repeat1($.node)),

    comment: _ => token(seq('//', /[^\r\n]*/)),

    // Node
    node: $ =>
      seq(
        field(
          'header',
          repeat1(choice($.title_header, $.when_header, $.header)),
        ),
        '---',
        field('body', alias(repeat($._statement), $.body)),
        '===',
      ),

    // Headers
    title_header: $ => seq('title', ':', field('title', $.identifier)),

    when_header: $ => seq('when', ':', field('expr', $.header_when_expression)),

    header_when_expression: $ =>
      choice(
        $._expression,
        'always',
        seq('once', optional(seq('if', $._expression))),
      ),

    header: $ =>
      prec.right(
        seq(
          field('key', $.identifier),
          ':',
          optional(field('value', $.header_value)),
        ),
      ),

    header_value: _ => token(/[^ \r\n].*/),

    // Statements
    _statement: $ =>
      choice(
        $.line_statement,
        $.shortcut_option_statement,
        $.line_group_statement,
        $._command_like_statements,
      ),

    _command_like_statements: $ =>
      seq(
        choice(
          $.if_statement,
          '<<endenum>>',
          $.set_statement,
          $.call_statement,
          $.declare_statement,
          $.enum_statement,
          $.jump_statement,
          '<<return>>',
          $.once_statement,
          $.command_statement,
        ),
        $._newline,
      ),

    // Line statement
    line_statement: $ =>
      seq(
        $._line_formatted_text,
        optional($.line_condition),
        repeat($.hashtag),
        $._newline,
      ),

    _line_formatted_text: $ => choice($.text, $._inline_expression),

    _inline_expression: $ => seq('{', $._expression, '}'),

    line_condition: $ =>
      choice(
        seq('<<', 'if', $._expression, '>>'),
        seq('<<once', optional(seq('if', $._expression)), '>>'),
      ),

    // If statement
    if_statement: $ =>
      seq(
        seq('<<', 'if', field('condition', $._expression), '>>'),
        field('consequence', alias(repeat($._statement), $.block)),
        optional(field('alternative', choice($.else_if_clause, $.else_clause))),
        '<<endif>>',
      ),

    else_if_clause: $ =>
      seq(
        repeat1(
          seq(
            seq('<<elseif', field('condition', $._expression), '>>'),
            field('consequence', alias(repeat($._statement), $.block)),
          ),
        ),
        field('alternative', optional($.else_clause)),
      ),

    else_clause: $ => seq('<<else>>', alias(repeat($._statement), $.block)),

    // Set statement
    set_statement: $ =>
      seq(
        '<<',
        'set',
        $.variable,
        field(
          'operator',
          choice(choice('=', 'to'), '+=', '-=', '*=', '/=', '%='),
        ),
        $._expression,
        '>>',
      ),

    // Call statement
    call_statement: $ => seq('<<', 'call', $.function_call, '>>'),

    // Command statement - generic commands that don't match specific patterns
    command_statement: $ =>
      seq('<<', $._command_statement_args, '>>', repeat($.hashtag)),

    _command_statement_args: $ =>
      repeat1(choice($.identifier, $.string, $.number, $._inline_expression)),

    // Declare statement
    declare_statement: $ =>
      seq(
        '<<',
        'declare',
        $.variable,
        choice('=', 'to'),
        $._expression,
        optional(seq('as', field('type', $.identifier))),
        '>>',
      ),

    // Enum statement
    enum_statement: $ =>
      seq(
        seq('<<', 'enum', field('name', $.identifier), '>>'),
        repeat1(seq('<<', $.enum_case_statement, '>>')),
        '<<endenum>>',
      ),

    enum_case_statement: $ =>
      seq(
        'case',
        field('name', $.identifier),
        optional(
          seq(
            field('operator', choice('=', 'to')),
            field('value', $._expression),
          ),
        ),
      ),

    // Jump statement
    jump_statement: $ =>
      choice(
        seq('<<', 'jump', choice($.identifier, $._inline_expression), '>>'),
        seq('<<', 'detour', choice($.identifier, $._inline_expression), '>>'),
      ),

    // Once statement
    once_statement: $ =>
      seq(
        seq(
          '<<',
          'once',
          optional(field('condition', seq('if', $._expression))),
          '>>',
        ),
        alias(repeat($._statement), $.once_primary_clause),
        optional(
          field(
            'alternative',
            seq(
              '<<else>>',
              alias(repeat($._statement), $.once_alternate_clause),
            ),
          ),
        ),
        '<<endonce>>',
      ),

    // Shortcut option statement
    shortcut_option_statement: $ => prec.right(seq(repeat1($.shortcut_option))),

    shortcut_option: $ =>
      seq(
        '->',
        field('text', alias($.line_statement, $.option_line)),
        optional(field('body', $.option_body)),
      ),

    option_body: $ => seq($._indent, repeat($._statement), $._dedent),

    // Line group statement
    line_group_statement: $ => seq(repeat1($.line_group_item)),

    line_group_item: $ =>
      seq(
        '=>',
        $.line_statement,
        // optional($._statement),
      ),

    // Expressions
    _expression: $ =>
      choice(
        $.paren_expression,
        $.binary_expression,
        $.unary_expression,
        $.function_call,
        $.member_expression,
        $.number,
        $.string,
        $.variable,
        $.identifier,
        'true',
        'false',
        'null',
      ),

    paren_expression: $ => seq('(', $._expression, ')'),

    unary_expression: $ =>
      prec.right(
        9,
        seq(field('operator', choice('-', '!', 'not')), $._expression),
      ),

    binary_expression: $ => {
      const table = [
        [2, choice('and', '&&')],
        [3, choice('or', '||')],
        [4, choice('xor', '^')],
        [5, choice('is', 'eq', 'neq', '==', '!=')],
        [6, choice('lte', 'gte', 'lt', 'gt', '<=', '>=', '<', '>')],
        [7, choice('+', '-')],
        [8, choice('*', '/', '%')],
      ];

      // @ts-ignore
      return choice(
        ...table.map(([precedence, operator]) =>
          prec.left(
            // @ts-ignore
            precedence,
            seq(
              field('left', $._expression),
              // @ts-ignore
              field('operator', operator),
              field('right', $._expression),
            ),
          ),
        ),
      );
    },

    function_call: $ =>
      seq(
        field('function', $.identifier),
        '(',
        optional(seq($._expression, repeat(seq(',', $._expression)))),
        ')',
      ),

    member_expression: $ =>
      seq(
        optional(field('type', $.identifier)),
        '.',
        field('member', $.identifier),
      ),

    // Literals
    number: _ => token(choice(/[0-9]+/, /[0-9]+\.[0-9]+/)),

    string: _ =>
      token(seq('"', repeat(choice(/[^"\\\r\n]/, /\\"/, /\\\\/)), '"')),

    // Identifiers
    variable: $ => seq('$', $.identifier),

    identifier: _ => token(/[a-zA-Z_][a-zA-Z0-9_]*/),

    // Hashtag
    hashtag: $ => seq('#', $._hashtag_text),

    _hashtag_text: _ => token(/[^\s#<>{}\r\n]+/),

    // Text
    text: _ => token(/[^ \t#<>{}\r\n\\][^#<>{}\r\n\\]*/),

    // Newline: required for scanner dedent detection
    _newline: _ => token(seq(optional('\r'), '\n')),
  },
});
