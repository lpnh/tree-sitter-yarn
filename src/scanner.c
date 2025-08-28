/*
 * Hey, future me. Yes, you, me. It's me, you.
 * I left a bunch of comments like this for you.
 * Just in case you forget what this beaUwUtiful pile of C code does.
 * No need to thank me, thank you.
 *
 * How do you tell a computer that whitespace actually means something?
 */

#include "tree_sitter/parser.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * We take care of two things here:
 *
 * 1. INDENT: Whitespace that pushes the text rightward into the void
 * 2. DEDENT: Whitespace that yanks the text back from said void
 *
 * These are the only two tokens this scanner produces. Everything else is
 * someone else's problem (and by that I mean us, but in a different file).
 */
enum TokenType {
  INDENT,
  DEDENT,
};

/*
 * The Scanner State
 *
 * Because someone has to remember something in this stateless world.
 *
 * A *stack* of "how indented we are right now" at each nesting level.
 * Every time we indent, we push. Every time we dedent, we pop.
 * It's like a really boring accordion.
 *
 * Example:
 *
 * -> First Option
 *     -> Nested Option     # indent = 4 spaces
 *         Alice: Hi!       # indent = 8 spaces
 *
 * The stack would contain: [4, 8]
 */
typedef struct {
  uint32_t *indents;        // Our stack of indentations (the actual numbers)
  uint32_t indent_count;    // 1 indent, 2 indents, 3 indents...
  uint32_t indent_capacity; // How much room we have in our stack (help T.T)
  uint32_t dedent_trail;    // How many DEDENT tokens we have queued up
} Scanner;

/*
 * Helper functions
 */

// Move forward one character, marking it as significant for parsing
static void advance(TSLexer *lexer) { lexer->advance(lexer, false); }

// Move forward one character, but ignore it
static void skip(TSLexer *lexer) { lexer->advance(lexer, true); }

// Is this whitespace? (newlines not included)
static bool is_whitespace(int32_t c) { return c == ' ' || c == '\t'; }

// Is this the end of line?
static bool is_newline(int32_t c) { return c == '\n' || c == '\r'; }

/*
 * Stack management functions
 *
 * Because manually managing a dynamic array in C is everyone's favorite hobby.
 */

// Push a new indentation level onto our stack
// This happens when we encounter more indentation than we've seen so far
static void push_indent(Scanner *scanner, uint32_t indent) {
  // The eternal dance of dynamic array growth
  if (scanner->indent_count >= scanner->indent_capacity) {
    // Double the capacity, or start with 4 if we're starting from scratch
    scanner->indent_capacity =
        scanner->indent_capacity > 0 ? scanner->indent_capacity * 2 : 4;

    // They grow up so fast
    scanner->indents =
        realloc(scanner->indents, scanner->indent_capacity * sizeof(uint32_t));
  }

  // Store the indent level (signed, sealed, delivered to the stack)
  scanner->indents[scanner->indent_count++] = indent;
}

// Pop the top indentation level from our stack
// This happens when we're processing dedents and need to close nesting levels
static uint32_t pop_indent(Scanner *scanner) {
  if (scanner->indent_count > 0) {
    return scanner->indents[--scanner->indent_count];
  }
  // We shouldn't reach this, but life finds a way
  return 0; // Returning 0 and crossing our fingers
}

// Peek at the top indentation level without removing it
// What's our current indentation level again? *checks notes*
static uint32_t peek_indent(Scanner *scanner) {
  if (scanner->indent_count > 0) {
    return scanner->indents[scanner->indent_count - 1];
  }
  // We're at the root level (no indentation, all the potential)
  return 0;
}

/*
 * Indent and Dedent Detection
 *
 * This is where the magic happens. And by magic, I mean:
 * 1. Check if we have DEDENTs in our backlog to process first
 * 2. Only process indentation at the start of lines (we have standards)
 * 3. Count whitespace characters like a very boring accountant
 * 4. Compare with current indentation level
 * 5. Emit INDENT if we're moving forward
 * 6. Calculate and queue DEDENTs if we're going backwards
 */
static bool scan_indent_dedent(Scanner *scanner, TSLexer *lexer,
                               const bool *valid_symbols) {
  // Process DEDENT tokens from our backlog first
  if (scanner->dedent_trail > 0 && valid_symbols[DEDENT]) {
    scanner->dedent_trail--; // Take one DEDENT from the trail
    lexer->result_symbol = DEDENT;
    return true; // Here's your DEDENT token
  }

  // As I said, only process indentation at the start of lines
  if (lexer->get_column(lexer) != 0) {
    return false; // Not my column, not my problem
  }

  // Time to count some whitespace
  uint32_t indent_level = 0;
  while (is_whitespace(lexer->lookahead)) {
    if (lexer->lookahead == ' ') {
      indent_level += 1; // One space = one unit of indentation
    } else if (lexer->lookahead == '\t') {
      // Tabs are worth 8 spaces. We are here to count, not to judge
      indent_level += 8;
    }
    skip(lexer); // Move past this whitespace character
  }

  // Skip empty lines and lines with only whitespace
  if (is_newline(lexer->lookahead) || lexer->eof(lexer)) {
    return false; // Nothing to see here, move along
  }

  // Quick check for comment-only lines
  // Comments don't participate in indentation structure
  if (lexer->lookahead == '/') {
    TSLexer saved_lexer = *lexer; // Save our position
    advance(lexer);
    if (lexer->lookahead == '/') {
      // It's a comment line, restore position and bail out
      *lexer = saved_lexer; // Rollback
      return false;         // Comments are not our department
    }
    *lexer = saved_lexer; // False alarm, restore and carry on
  }

  // Now the real work begins: comparing indentation levels
  uint32_t current_indent = peek_indent(scanner);

  // Case 1: We're indenting more than before (moving forward)
  if (indent_level > current_indent && valid_symbols[INDENT]) {
    // Push this new indentation level onto our stack for future reference
    push_indent(scanner, indent_level);
    lexer->result_symbol = INDENT; // Your INDENT, sir
    return true;                   // Forward we go, into the whitespace unknown
  }

  // Case 2: We're indenting less than before (going back to our roots)
  if (indent_level < current_indent && valid_symbols[DEDENT]) {
    // Pop indent levels until we find one that matches our current indentation
    // or until we run out of levels, which probably means some wonky formatting
    while (scanner->indent_count > 0 && peek_indent(scanner) > indent_level) {
      pop_indent(scanner);     // Close this nesting level
      scanner->dedent_trail++; // Add a DEDENT to our backlog
    }

    // Got DEDENTs in our backlog? Let's process them
    if (scanner->dedent_trail > 0) {
      scanner->dedent_trail--; // Take one DEDENT from the trail (déjà vu)
      lexer->result_symbol = DEDENT;
      return true; // One DEDENT served, fresh from the queue
    }
  }

  // Case 3: Indentation level is the same as before
  return false; // ¯\_(ツ)_/¯
}

/*
 * Lifecycle management - the boring but absolutely critical stuff
 *
 * Tree-sitter will call these functions to create, destroy, and manage
 * our scanner instance.
 */

// Create a new scanner - fresh out of the factory
// This is called once when Tree-sitter first loads our scanner
void *tree_sitter_yarn_external_scanner_create() {
  // Allocate memory for our scanner state, initialized to zeros
  Scanner *scanner = calloc(1, sizeof(Scanner));

  // Initialize everything to safe default values
  // (calloc already zeroed everything, but being explicit prevents future
  // headaches)
  scanner->indents = NULL;      // No indent stack yet
  scanner->indent_count = 0;    // No indentation levels tracked
  scanner->indent_capacity = 0; // No memory allocated yet
  scanner->dedent_trail = 0;    // No DEDENTs in our backlog yet

  return scanner; // A shiny new scanner, handle with care
}

// Clean up after ourselves - because memory leaks are not cool
// This is called when Tree-sitter is done with our scanner (usually on exit)
void tree_sitter_yarn_external_scanner_destroy(void *payload) {
  Scanner *scanner = (Scanner *)payload;

  if (scanner) {
    if (scanner->indents) {
      free(scanner->indents); // Free the indent stack memory
    }
    free(scanner); // Free the scanner itself
  }

  // And now our watch has ended
}

/*
 * Serialization - saving our progress for later
 *
 * Because sometimes the parser needs to backtrack (like when it realizes it
 * made a bad choice and needs to try a different parsing path), and we need to
 * remember where we were when that happened.
 *
 * This is like creating a checkpoint, but for parsing state.
 * Tree-sitter handles when to save/restore; we just handle the how.
 */
unsigned tree_sitter_yarn_external_scanner_serialize(void *payload,
                                                             char *buffer) {
  Scanner *scanner = (Scanner *)payload;
  if (!scanner)
    return 0; // Nothing to save, nothing to lose

  // Calculate how much space we need for our stuff
  // We need to save indent_count, dedent_trail, and the indent levels
  size_t size = sizeof(uint32_t) * 2; // Space for indent_count + dedent_trail
  size += scanner->indent_count *
          sizeof(uint32_t); // Space for all the indent levels

  // Make sure we fit in Tree-sitter's provided buffer
  // If we don't fit, we can't save (this would be... rare)
  if (size > TREE_SITTER_SERIALIZATION_BUFFER_SIZE) {
    return 0; // That's too much, man
  }

  // Pack everything into the buffer (like Tetris)
  size_t pos = 0;

  // Save how many indent levels we have
  memcpy(buffer + pos, &scanner->indent_count, sizeof(uint32_t));
  pos += sizeof(uint32_t);

  // Save how many DEDENTs are in our backlog
  memcpy(buffer + pos, &scanner->dedent_trail, sizeof(uint32_t));
  pos += sizeof(uint32_t);

  // Save the actual indent levels (the stack contents)
  for (uint32_t i = 0; i < scanner->indent_count; i++) {
    memcpy(buffer + pos, &scanner->indents[i], sizeof(uint32_t));
    pos += sizeof(uint32_t);
  }

  return pos; // *chef's kiss*
}

// Deserialization - here we restore our state, like loading the checkpoint
void tree_sitter_yarn_external_scanner_deserialize(void *payload,
                                                           const char *buffer,
                                                           unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  if (!scanner)
    return; // No scanner, no state

  // Handle the "empty save" case (clean slate)
  // Starting fresh, no previous state to restore
  if (length == 0) {
    scanner->indent_count = 0;
    scanner->dedent_trail = 0;
    return;
  }

  // Unpack the buffer (reverse Tetris)
  size_t pos = 0;

  // Restore how many indent levels we had
  memcpy(&scanner->indent_count, buffer + pos, sizeof(uint32_t));
  pos += sizeof(uint32_t);

  // Restore how many DEDENTs are in our backlog
  memcpy(&scanner->dedent_trail, buffer + pos, sizeof(uint32_t));
  pos += sizeof(uint32_t);

  // Restore the indent stack, if we had one
  if (scanner->indent_count > 0) {
    // Allocate exactly the memory we need (no extra capacity this time)
    scanner->indent_capacity = scanner->indent_count;
    scanner->indents = malloc(scanner->indent_capacity * sizeof(uint32_t));

    // Restore each indent level
    for (uint32_t i = 0; i < scanner->indent_count; i++) {
      memcpy(&scanner->indents[i], buffer + pos, sizeof(uint32_t));
      pos += sizeof(uint32_t);
    }
  }

  // State successfully restored, let's go!
}

/*
 * The main scanner function
 *
 * This is the function Tree-sitter calls when it wants to know:
 * "Hey, can you handle any of these tokens?"
 *
 * Return true = Yes, there's a token here somewhere, check lexer->result_symbol
 * Return false = Nope, go check elsewhere
 */
bool tree_sitter_yarn_external_scanner_scan(void *payload,
                                                    TSLexer *lexer,
                                                    const bool *valid_symbols) {
  // Where's my scanner? There's my scanner
  Scanner *scanner = (Scanner *)payload;
  if (!scanner)
    return false; // Can't work without my scanner

  // When we reach EOF, we need to close any remaining open indentation levels
  if (lexer->eof(lexer)) {
    // Still got DEDENTs in our backlog? Process them first
    if (scanner->dedent_trail > 0 && valid_symbols[DEDENT]) {
      scanner->dedent_trail--;
      lexer->result_symbol = DEDENT;
      return true; // Here's a DEDENT for EOF
    }

    // Do we still have open indentation levels? Close them one by one
    if (scanner->indent_count > 0 && valid_symbols[DEDENT]) {
      pop_indent(scanner); // Close one level
      lexer->result_symbol = DEDENT;
      return true; // I can't go on, I'll go on
    }

    return false; // EOF reached, all indentation levels closed, life's good
  }

  // Normal case: try to handle indent/dedent tokens
  if (scan_indent_dedent(scanner, lexer, valid_symbols)) {
    return true; // Done
  }

  // Sorry, no tokens for you today
  return false;
}
