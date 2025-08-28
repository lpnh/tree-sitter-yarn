/**
 * @file Yarn grammar for tree-sitter
 * @author lpnh <paniguel.lpnh@gmail.com>
 * @license Unlicense
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "yarn",

  rules: {
    // TODO: add the actual grammar rules
    source_file: $ => "hello"
  }
});
