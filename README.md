# AVL-Tree-Deletion-Rebalancing
DSA-II Group Project — AVL Tree Implementation with Full Deletion Rebalancing in C++

## Architecture

`include/AVLNode.h` defines the shared node type. `include/AVLTree.h` is the
stable interface for the implementation modules. `AVLTree` exclusively owns
its nodes, releases them in its destructor, and cannot be copied. Call
`clear()` to release the current contents and leave the tree empty.

### Shared interface

```cpp
AVLTree() noexcept;
~AVLTree() noexcept;
void insert(int key);
bool remove(int key);
bool search(int key) const noexcept;
std::vector<int> inorderTraversal() const;
std::vector<int> preorderTraversal() const;
std::vector<int> postorderTraversal() const;
int getHeight() const noexcept;
int getBalanceFactor() const noexcept;
void clear() noexcept;
```

`insert` ignores duplicate keys. `remove` returns `true` only when it removes
an existing key, and `search` reports whether a key is present. Traversals
return keys in the named order. A null subtree has height `0`, a leaf has
height `1`, and balance factor is `height(left) - height(right)`; therefore a
valid AVL node has a balance factor from `-1` through `1`.

The private helpers in `AVLTree.h` are shared implementation hooks: Aditya
implements rotations and rebalancing in `src/AVLBalance.cpp`; Shubham implements
insertion/search and traversals/utilities in `src/AVLInsert.cpp` and
`src/AVLTraversal.cpp`; Alina implements removal and upward rebalancing in
`src/AVLDelete.cpp`. Keep these definitions consistent with the header and
avoid duplicating algorithms across modules. This architecture change provides
no insertion, removal, search, or traversal algorithm implementations.

## Build and verification

Requires CMake 3.16 or newer and a C++17 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The current tests verify node initialization, empty-tree height/balance,
idempotent cleanup, and non-copyable ownership. Algorithm behavior remains
unimplemented until the assigned modules are completed.
