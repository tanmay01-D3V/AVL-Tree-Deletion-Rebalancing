# AVL Tree Deletion Rebalancing

DSA-II group project: a C++17 AVL tree with insertion, search, deletion,
traversals, and an interactive console application. The tree keeps keys in
binary-search-tree order and rebalances after insertions and deletions.

## Features

- Insert unique integer keys; duplicate insertions are ignored.
- Search for a key and remove a key, with a success result for deletion.
- Rebalance using LL, RR, LR, and RL rotations. Deletion rebalances every
  affected ancestor.
- Display inorder, preorder, and postorder traversals, tree height, and the
  root balance factor.
- Manage owned nodes with `AVLTree`; copying is disabled and destruction or
  `clear()` releases the tree.

A null subtree has height `0`, a leaf has height `1`, and balance factor is
`height(left) - height(right)`. A balanced node has a factor from `-1` through
`1`.

## Team responsibilities and contribution record

Issue assignments and commit authorship differ in places. GitHub currently
records these responsibilities:

- **Tanmay Vijay Sherkar:** architecture (Issue #1), console application and
  integration (Issue #6), and coordination of shared validation/documentation
  (Issue #7).
- **Alina Patel:** rotations and balancing helpers (Issue #2).
- **Shubham Mane:** insertion/search (Issue #3) and traversals/utilities
  (Issue #5).
- **Aditya Kadam:** deletion with upward rebalancing (Issue #4).
- **All members:** shared testing and documentation (Issue #7).

PR #15 was authored by Aditya Kadam and includes the balancing and deletion
implementations, as well as insertion/search, traversal, and test files used to
exercise deletion. Confirm final contribution attribution with the team;
GitHub assignments alone do not establish who wrote each module.

## Project structure

```text
include/AVLNode.h       AVL node definition
include/AVLTree.h       Public tree interface and private helper declarations
src/AVLTree.cpp         Ownership, height, balance-factor, and cleanup logic
src/AVLBalance.cpp      Rotations and rebalancing
src/AVLInsert.cpp       Insertion and search
src/AVLDelete.cpp       Removal and upward rebalancing
src/AVLTraversal.cpp    Inorder, preorder, and postorder traversals
tests/tests.cpp         AVL behavior and invariant tests
main.cpp                Interactive console application
CMakeLists.txt          Library, application, and test build configuration
```

## Prerequisites

- CMake 3.16 or newer
- A C++17-compatible compiler

## Build and run

From the repository root:

```sh
cmake -S . -B build
cmake --build build
./build/avl_demo
```

On Windows, run `build\avl_demo.exe` instead.

## Menu

1. Insert an integer.
2. Delete an integer.
3. Search for an integer.
4. Display inorder traversal.
5. Display preorder traversal.
6. Display postorder traversal.
7. Display tree height.
8. Display the root balance factor.
9. Display tree contents in inorder.
10. Exit.

Input is read one line at a time. Invalid menu choices and non-integer values
are rejected without terminating the application. Empty-tree operations report
the empty state (height is `0`, and root balance factor is `0`). Duplicate
keys are ignored and reported.

## Tests

Run the CTest suite after building:

```sh
ctest --test-dir build --output-on-failure
```

Tests cover all four rotation patterns, insertion and duplicate handling,
search, leaf/one-child/two-child and root deletion, deletion rebalancing,
traversal order, empty-tree behavior, and repeated mutations. The recursive
invariant checker verifies BST ordering, every stored node height, and every
node's balance factor after mutation sequences.

## Known limitations

- The console application operates on integer keys and does not persist data
  after exit.
- The public balance-factor API reports the root's balance factor; the
  recursive test checker inspects all nodes.
- The tree is intended for single-threaded use.
