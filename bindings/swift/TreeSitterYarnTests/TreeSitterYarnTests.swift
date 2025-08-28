import XCTest
import SwiftTreeSitter
import TreeSitterYarn

final class TreeSitterYarnTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_yarn())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading Yarn grammar")
    }
}
