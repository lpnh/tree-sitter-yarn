package tree_sitter_yarn_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_yarn "github.com/lpnh/tree-sitter-yarn/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_yarn.Language())
	if language == nil {
		t.Errorf("Error loading Yarn grammar")
	}
}
