// swift-tools-version:5.3

import Foundation
import PackageDescription

var sources = ["src/parser.c"]
if FileManager.default.fileExists(atPath: "src/scanner.c") {
    sources.append("src/scanner.c")
}

let package = Package(
    name: "TreeSitterYarn",
    products: [
        .library(name: "TreeSitterYarn", targets: ["TreeSitterYarn"]),
    ],
    dependencies: [
        .package(name: "SwiftTreeSitter", url: "https://github.com/tree-sitter/swift-tree-sitter", from: "0.9.0"),
    ],
    targets: [
        .target(
            name: "TreeSitterYarn",
            dependencies: [],
            path: ".",
            sources: sources,
            resources: [
                .copy("queries")
            ],
            publicHeadersPath: "bindings/swift",
            cSettings: [.headerSearchPath("src")]
        ),
        .testTarget(
            name: "TreeSitterYarnTests",
            dependencies: [
                "SwiftTreeSitter",
                "TreeSitterYarn",
            ],
            path: "bindings/swift/TreeSitterYarnTests"
        )
    ],
    cLanguageStandard: .c11
)
