# Speaker Scripts — AVL Tree Presentation

**DSA Mini Project · AY 2026 · Semester 3 · Use with `PRESENTATION_SCRIPT.md`**

> Each member speaks only their own slides. **Show** = file + class to open in the editor. **Live** = action at `http://127.0.0.1:8080/` (server: `./build/avl_server`) or in `./build/avl_demo`. Keep each slide to about 45–60 seconds. No reservation pages, no frontend screens.

---

## Member 1 — Tanmay (Project Lead)
*Setup, architecture, console menu, integration — Slides 1, 3, 10*

### Slide 1 — Title & Roadmap
"Good morning. Our project is an AVL tree — a self-balancing binary search tree named after Adelson-Velsky and Landis. We didn't just want insertion and deletion to work; we wanted to show *why* the tree stays shallow: each rotation fixes height without breaking key order. We'll move from the BST imbalance problem, through our architecture, to height and rotations, then insertion, traversals, deletion, and finally complexity."
- **Show:** `README.md`, `CMakeLists.txt`, class `AVLTree` in `include/AVLTree.h`, `main()` in `main.cpp`
- **Live:** Open `/` and leave the visualizer on screen.

### Slide 3 — Architecture
"We kept the data structure independent of any application. One shared library holds all AVL logic, split by responsibility: tree state, balance, insertion, deletion, traversals. A rotation is a primitive; insertion and deletion are its clients; traversals only read. One implementation of balance means a rotation bug can't hide in two places. I defined the boundaries, wired the build, and integrated everyone's module."
- **Show:** `CMakeLists.txt` (targets `avl_tree`, `avl_demo`, `avl_server`), `cmake/AVLTreeSources.cmake`, `include/AVLTree.h` + `src/AVLTree.cpp` (classes `AVLTree`, `AVLNode`)
- **Live:** Run `ctest --test-dir build --output-on-failure` and show the printed times.

### Slide 10 — Correctness, Complexity & Conclusion
"Our correctness argument is short: every rotation preserves inorder order, and every balance factor stays within minus one to plus one — so the structure is always a valid BST and a valid AVL tree. Tests include randomized checks against `std::set` across all rotations, all deletion cases, and multi-ancestor rebalancing. Height is about 1.44 log₂(n+2), so search, insert and delete are O(log n), traversals O(n), each rotation O(1). A local rule gives a global guarantee. Thank you — we'll take questions."
- **Show:** `tests/tests.cpp` (`AVLTreeInvariantTestAccess`), README "Tests" section
- **Live:** `ctest --test-dir build --output-on-failure`; all four members join for Q&A.

---

## Member 2 — Alina (AVL Rotations)
*Height, balance factors, LL/RR/LR/RL — Slides 4, 5, 6*

### Slide 4 — Height & Balance Factor
"Height counts levels to the deepest leaf, with an empty subtree at zero and a leaf at one. Balance factor is left height minus right height: minus one, zero, plus one are allowed; magnitude two means rotate. Think of it as a tilt meter on *every* node, not just the root. We store height at each node so the check is constant time, and recompute it on the way back to the root after an update."
- **Show:** `include/AVLNode.h` (`height`), `include/AVLTree.h` (`nodeHeight`, `nodeBalanceFactor`, `updateHeight`, `getHeight`, `getBalanceFactor`, `AVLTreeNodeSnapshot`), `src/AVLTree.cpp`
- **Live:** Point at SVG node labels and the "Height" / "Root bf" cards. Console menu **7** and **8**.

### Slide 5 — LL & RR
"LL means a left-heavy node with a left-heavy child — one right rotation promotes the child and demotes the old root. RR is the mirror: one left rotation. Only three pointers move; no data is copied, and sorted order survives because the promoted child's right subtree sits between the two keys. That's the geometric heart of AVL: one local pivot fixes the tilt."
- **Show:** `src/AVLBalance.cpp` (`rotateLeft`, `rotateRight`, `rebalance`), `AVLRotationType` in `include/AVLTree.h`
- **Live:** Reset ↻ → **LL**, then reset → **RR**. Console: insert `30,20,10` and `10,20,30`.

### Slide 6 — LR & RL
"When the tilt is zig-zag, a single rotation won't do. LR rotates left at the child, then right at the grandparent; RL is the symmetric right-then-left. A double rotation is still just two single rotations — constant time, order-preserving. Crucially, it restores the subtree's previous height, so the height change does not travel further up. That's why insertion needs at most one rotation event."
- **Show:** same as slide 5 — `src/AVLBalance.cpp`, `AVLRotationType`
- **Live:** Reset → **LR**, reset → **RL**. Console: `30,10,20` and `10,30,20`.

---

## Member 3 — Shubham (Insertion & Traversals)
*Insertion, searching, traversals — Slides 2, 7, 8*

### Slide 2 — The Problem
"An ordinary BST promises O(log n) search, but only if it's roughly balanced. Feed it sorted keys — 10, 20, 30, 40, 50 — and it becomes a linked list with only right children; searching 50 takes five comparisons instead of three. AVL removes that dependency on input order by enforcing |balance factor| ≤ 1 at every node. We pay a few pointer changes on update and get a logarithmic worst case for *every* input."
- **Show:** `include/AVLNode.h`, `src/AVLInsert.cpp` (`insertNode`)
- **Live:** Reset ↻, insert `10, 20, 30, 40, 50`; point at the shallow tree and the rotation log.

### Slide 7 — Insertion & Search
"Insertion starts as ordinary BST descent to a new leaf. The AVL part is the return trip: each ancestor updates height and checks balance; at the first ±2 we identify LL, RR, LR or RL, rotate, and stop — because that rotation restores the old height, so nothing above can be unbalanced. Search needs no changes at all: rotations preserve key order, so compare-and-descend still works in O(log n). Balancing is invisible to reads."
- **Show:** `src/AVLInsert.cpp`, `include/AVLTree.h` (`insert`, `search`, `containsNode`), `main.cpp` options **1**, **3**
- **Live:** Use "Integer key" → **Insert key**, then "Search with C++" → **Find**; show the sorted inorder strip.

### Slide 8 — Traversals
"Three orders, three purposes. Inorder — left, root, right — yields sorted keys; for us it's proof the rotations kept BST order. Preorder — root, left, right — visits a parent before children, right for copying or serializing. Postorder — left, right, root — visits children first, the natural order for safe deletion and bottom-up height. All are O(n): every node is visited once."
- **Show:** `src/AVLTraversal.cpp`, `include/AVLTree.h` (`inorderTraversal`, `preorderTraversal`, `postorderTraversal`), `main.cpp` options **4**, **5**, **6**
- **Live:** Visualizer shows inorder live. For preorder and postorder use console menu **5** and **6**.

---

## Member 4 — Aditya (Deletion)
*Deletion for all cases, full upward rebalancing — Slide 9*

### Slide 9 — Deletion & Upward Rebalancing
"Deletion is hardest because removing a node can *shrink* a subtree's height, unbalancing ancestors that were fine before. First do the BST removal: a leaf is detached, a one-child node is replaced by its child, and a two-child node is replaced by its inorder successor — the smallest key in the right subtree — which is then removed from its old spot. Then walk from the parent all the way to the root, updating heights and rebalancing at every node. Unlike insertion, a rotation here may lower the height by one, so the next ancestor can become unbalanced too — one deletion can need several rotations, up to O(log n)."
- **Show:** `src/AVLDelete.cpp`, `include/AVLTree.h` (`remove`, `removeNode`), `main.cpp` option **2**
- **Live:** Delete a leaf, a one-child node, and a two-child node; point to multiple entries in the operation log to show rebalancing at several ancestors.

---

**Common closing:** keep the talk conceptual, point to the diagram first and the code second, and hand over to Member 1 for slide 10's Q&A.
