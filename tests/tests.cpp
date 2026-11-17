// Tests must keep assert() active even in Release builds, where CMake defines
// NDEBUG. Undefine it before <cassert> is included so invariant checks always run.
#ifdef NDEBUG
#undef NDEBUG
#endif

#include "AVLNode.h"
#include "AVLTree.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <type_traits>
#include <vector>

struct AVLTreeInvariantTestAccess {
    static bool hasValidInvariants(const AVLTree& tree) noexcept {
        int calculatedHeight = 0;
        return validateNode(tree.root_, nullptr, nullptr, calculatedHeight)
            && calculatedHeight == tree.getHeight();
    }

private:
    static bool validateNode(const AVLNode* node,
                             const int* lowerBound,
                             const int* upperBound,
                             int& calculatedHeight) noexcept {
        if (node == nullptr) {
            calculatedHeight = 0;
            return true;
        }
        if ((lowerBound != nullptr && node->key <= *lowerBound)
            || (upperBound != nullptr && node->key >= *upperBound)) {
            return false;
        }

        int leftHeight = 0;
        int rightHeight = 0;
        if (!validateNode(node->left, lowerBound, &node->key, leftHeight)
            || !validateNode(node->right, &node->key, upperBound, rightHeight)) {
            return false;
        }

        calculatedHeight = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
        const int balanceFactor = leftHeight - rightHeight;
        return node->height == calculatedHeight
            && -1 <= balanceFactor && balanceFactor <= 1;
    }
};

namespace {

// ---- Shared checks ---------------------------------------------------------

void assertTreeInvariants(const AVLTree& tree) {
    assert(AVLTreeInvariantTestAccess::hasValidInvariants(tree));
}

// Confirms the inorder output is sorted and matches the expected contents.
void assertInorder(const AVLTree& tree, const std::vector<int>& expected) {
    assertTreeInvariants(tree);
    const std::vector<int> actual = tree.inorderTraversal();
    if (!std::is_sorted(actual.begin(), actual.end()) || actual != expected) {
        std::printf("FAIL: inorder {");
        for (int key : actual) {
            std::printf("%d ", key);
        }
        std::printf("} expected {");
        for (int key : expected) {
            std::printf("%d ", key);
        }
        std::printf("}\n");
    }
    assert(std::is_sorted(actual.begin(), actual.end()));
    assert(actual == expected);
}

// Exact expected height plus a root balance factor inside [-1, 1].
void assertRootState(const AVLTree& tree, int expectedHeight) {
    assertTreeInvariants(tree);
    if (tree.getHeight() != expectedHeight) {
        std::printf("FAIL: height %d expected %d (bf %d)\n",
                    tree.getHeight(), expectedHeight, tree.getBalanceFactor());
    }
    assert(tree.getHeight() == expectedHeight);
    const int balanceFactor = tree.getBalanceFactor();
    assert(-1 <= balanceFactor && balanceFactor <= 1);
}

void assertRotation(AVLTree& tree,
                    AVLRotationType expectedType,
                    int expectedPivot) {
    const std::vector<AVLRotationEvent> events = tree.takeRotationEvents();
    assert(events.size() == 1);
    assert(events[0].type == expectedType);
    assert(events[0].pivotKey == expectedPivot);
}

// Public-API AVL invariant check for a tree holding `size` nodes: the height
// must respect the structural AVL bounds and the root balance factor must
// stay within [-1, 1].
void assertHeightBounds(const AVLTree& tree, int size) {
    assert(size > 0);
    const double height = static_cast<double>(tree.getHeight());
    if (height < std::ceil(std::log2(size + 1.0)) || height > 2.0 * std::log2(size + 1.0)) {
        std::printf("FAIL: height %.0f out of bounds for size %d\n", height, size);
    }
    assert(height >= std::ceil(std::log2(size + 1.0)));
    assert(height <= 2.0 * std::log2(size + 1.0));
    assert(-1 <= tree.getBalanceFactor() && tree.getBalanceFactor() <= 1);
}

std::vector<int> range(int from, int to) {
    std::vector<int> values;
    for (int key = from; key <= to; ++key) {
        values.push_back(key);
    }
    return values;
}

void makePerfectSeven(AVLTree& tree) {
    tree.insert(4);
    tree.insert(2);
    tree.insert(6);
    tree.insert(1);
    tree.insert(3);
    tree.insert(5);
    tree.insert(7);
}

// ---- Architecture contract (existing tests) --------------------------------

void testOwnershipContract() {
    static_assert(!std::is_copy_constructible<AVLTree>::value,
                  "AVLTree owns its nodes and must not be copied.");
    static_assert(!std::is_copy_assignable<AVLTree>::value,
                  "AVLTree owns its nodes and must not be copy-assigned.");

    const AVLNode node(42);
    assert(node.key == 42);
    assert(node.left == nullptr);
    assert(node.right == nullptr);
    assert(node.height == 1);

    AVLTree tree;
    assert(tree.getHeight() == 0);
    assert(tree.getBalanceFactor() == 0);
    tree.clear();
    tree.clear();
    assert(tree.getHeight() == 0);
}

void testSnapshot() {
    AVLTree tree;
    tree.insert(2);
    tree.insert(1);
    tree.insert(3);

    const std::vector<AVLTreeNodeSnapshot> nodes = tree.snapshot();
    assert(nodes.size() == 3);
    assert(nodes[0].key == 2);
    assert(nodes[0].leftIndex == 1);
    assert(nodes[0].rightIndex == 2);
    assert(nodes[0].height == 2);
    assert(nodes[0].balanceFactor == 0);
    assert(nodes[1].key == 1);
    assert(nodes[1].leftIndex == -1 && nodes[1].rightIndex == -1);
    assert(nodes[2].key == 3);
    assert(nodes[2].leftIndex == -1 && nodes[2].rightIndex == -1);

    tree.clear();
    assert(tree.snapshot().empty());
}

// ---- Insertion and search --------------------------------------------------

void testInsertionAndSearch() {
    AVLTree tree;
    tree.insert(42);
    assertRootState(tree, 1);
    assertInorder(tree, {42});
    assert(tree.search(42));
    assert(!tree.search(43));

    // Duplicate keys are ignored (documented policy).
    tree.insert(42);
    tree.insert(41);
    assertRootState(tree, 2);
    assertInorder(tree, {41, 42});
    assert(tree.search(41) && tree.search(42));

    // Sorted, reverse-sorted, and mixed sequences stay balanced and sorted.
    AVLTree sorted;
    for (int key = 1; key <= 100; ++key) {
        sorted.insert(key);
    }
    assertHeightBounds(sorted, 100);
    assertInorder(sorted, range(1, 100));
    assert(sorted.search(50));
    assert(!sorted.search(1000));

    AVLTree reverse;
    for (int key = 100; key >= 1; --key) {
        reverse.insert(key);
    }
    assertHeightBounds(reverse, 100);
    assertInorder(reverse, range(1, 100));

    // (key * 37) % 101 is a bijection over 1..100: a shuffled unique sequence.
    AVLTree mixed;
    for (int key = 1; key <= 100; ++key) {
        mixed.insert((key * 37) % 101);
    }
    assertHeightBounds(mixed, 100);
    assertInorder(mixed, range(1, 100));
}

void testTraversalOrdersAndEmptyTree() {
    AVLTree tree;
    assert(tree.inorderTraversal().empty());
    assert(tree.preorderTraversal().empty());
    assert(tree.postorderTraversal().empty());
    assertTreeInvariants(tree);

    tree.insert(2);
    tree.insert(1);
    tree.insert(3);
    assert(tree.inorderTraversal() == std::vector<int>({1, 2, 3}));
    assert(tree.preorderTraversal() == std::vector<int>({2, 1, 3}));
    assert(tree.postorderTraversal() == std::vector<int>({1, 3, 2}));
    assertTreeInvariants(tree);
}

void testRecursiveInvariantsAfterMutations() {
    AVLTree tree;
    std::vector<int> expected;

    for (int key = 1; key <= 100; ++key) {
        const int value = (key * 37) % 101;
        tree.insert(value);
        expected.push_back(value);
        std::sort(expected.begin(), expected.end());
        assertTreeInvariants(tree);
        assertInorder(tree, expected);
    }

    for (int key = 1; key <= 100; ++key) {
        assert(tree.remove(key));
        expected.erase(std::lower_bound(expected.begin(), expected.end(), key));
        assertTreeInvariants(tree);
        assertInorder(tree, expected);
    }

    assert(!tree.remove(1000));
    assertTreeInvariants(tree);
    assertInorder(tree, {});
}

// All four insertion rotation patterns collapse to the same balanced shape.
void testInsertionRotations() {
    AVLTree ll;
    ll.insert(3);
    ll.insert(2);
    ll.insert(1);
    assertRotation(ll, AVLRotationType::Right, 3);
    assertRootState(ll, 2);
    assertInorder(ll, {1, 2, 3});

    AVLTree rr;
    rr.insert(1);
    rr.insert(2);
    rr.insert(3);
    assertRotation(rr, AVLRotationType::Left, 1);
    assertRootState(rr, 2);
    assertInorder(rr, {1, 2, 3});

    AVLTree lr;
    lr.insert(3);
    lr.insert(1);
    lr.insert(2);
    assertRotation(lr, AVLRotationType::LeftRight, 3);
    assertRootState(lr, 2);
    assertInorder(lr, {1, 2, 3});

    AVLTree rl;
    rl.insert(1);
    rl.insert(3);
    rl.insert(2);
    assertRotation(rl, AVLRotationType::RightLeft, 1);
    assertRootState(rl, 2);
    assertInorder(rl, {1, 2, 3});

    AVLTree perfect;
    for (int key = 1; key <= 7; ++key) {
        perfect.insert(key);
    }
    assertRootState(perfect, 3);
    assert(perfect.getBalanceFactor() == 0);
    assertInorder(perfect, {1, 2, 3, 4, 5, 6, 7});
}

// ---- Deletion: structural cases --------------------------------------------

void testDeletionStructuralCases() {
    // Missing key on an empty tree leaves the tree unchanged.
    AVLTree tree;
    assert(!tree.remove(5));
    assertRootState(tree, 0);

    // Perfect tree of seven keys for the structural cases.
    tree.insert(4);
    tree.insert(2);
    tree.insert(6);
    tree.insert(1);
    tree.insert(3);
    tree.insert(5);
    tree.insert(7);
    assertRootState(tree, 3);
    assert(tree.getBalanceFactor() == 0);

    // Missing key on a non-empty tree leaves the tree unchanged.
    assert(!tree.remove(99));
    assertRootState(tree, 3);
    assertInorder(tree, {1, 2, 3, 4, 5, 6, 7});

    // Leaf deletion; removing the same key again is a no-op.
    assert(tree.remove(1));
    assertRootState(tree, 3);
    assertInorder(tree, {2, 3, 4, 5, 6, 7});
    assert(!tree.remove(1));

    // Root deletion, two-children case: the inorder successor 5 replaces 4.
    assert(tree.remove(4));
    assertRootState(tree, 3);
    assertInorder(tree, {2, 3, 5, 6, 7});

    // Root deletion, one-child case.
    AVLTree oneChildRoot;
    oneChildRoot.insert(2);
    oneChildRoot.insert(1);
    assert(oneChildRoot.remove(2));
    assertRootState(oneChildRoot, 1);
    assertInorder(oneChildRoot, {1});

    // Root deletion, leaf case.
    AVLTree leafRoot;
    leafRoot.insert(1);
    assert(leafRoot.remove(1));
    assertRootState(leafRoot, 0);
    assertInorder(leafRoot, {});

    // Inner node with one child.
    AVLTree innerOneChild;
    innerOneChild.insert(3);
    innerOneChild.insert(1);
    innerOneChild.insert(4);
    innerOneChild.insert(2); // Shape: 3(1(-,2),4)
    assert(innerOneChild.remove(1));
    assertRootState(innerOneChild, 2);
    assertInorder(innerOneChild, {2, 3, 4});
}

// ---- Deletion: rebalancing cases -------------------------------------------

void testDeletionTriggeredSingleRotation() {
    // Build 2(1,4(3,5)). Deleting leaf 1 makes the root right-heavy by two
    // while its right child's balance factor is zero: a single left rotation.
    AVLTree tree;
    tree.insert(2);
    tree.insert(1);
    tree.insert(3);
    tree.insert(4);
    tree.insert(5);
    assertRootState(tree, 3);

    assert(tree.remove(1));
    assertRotation(tree, AVLRotationType::Left, 2);
    assertRootState(tree, 3);
    assertInorder(tree, {2, 3, 4, 5});

    // Repeated deletion until the tree becomes empty.
    assert(tree.remove(3));
    assertRootState(tree, 2);
    assertInorder(tree, {2, 4, 5});
    assert(tree.remove(2));
    assertRootState(tree, 2);
    assertInorder(tree, {4, 5});
    assert(tree.remove(5));
    assertRootState(tree, 1);
    assertInorder(tree, {4});
    assert(tree.remove(4));
    assertRootState(tree, 0);
    assertInorder(tree, {});
}

void testDeletionTriggeredRightRotation() {
    // Perfect tree minus its right leaves: deleting 6 makes the root
    // left-heavy by two while its left child's balance factor is zero:
    // a single right rotation.
    AVLTree tree;
    makePerfectSeven(tree);
    assert(tree.remove(5));
    assert(tree.remove(7));
    assertRootState(tree, 3);

    assert(tree.remove(6));
    assertRotation(tree, AVLRotationType::Right, 4);
    assertRootState(tree, 3);
    assertInorder(tree, {1, 2, 3, 4});
}

void testDeletionTriggeredDoubleRotation() {
    // Shape 3(1,5(4)). Deleting leaf 1 makes the root right-heavy by two with
    // a right child that leans left: an RL double rotation.
    AVLTree tree;
    tree.insert(3);
    tree.insert(1);
    tree.insert(5);
    tree.insert(4);
    assertRootState(tree, 3);

    assert(tree.remove(1));
    assertRotation(tree, AVLRotationType::RightLeft, 3);
    assertRootState(tree, 2);
    assertInorder(tree, {3, 4, 5});
}

void testDeletionRebalancesMultipleAncestors() {
    // Shape 5(3(1(-,2),4),8(7(6),9)). Deleting leaf 4 unbalances node 3
    // (LR double rotation). Deleting leaf 2 and then leaf 1 shrinks the left
    // subtree further until the root itself unbalances (RL double rotation),
    // so rebalancing runs and rotates at more than one ancestor level.
    AVLTree tree;
    tree.insert(5);
    tree.insert(3);
    tree.insert(8);
    tree.insert(1);
    tree.insert(4);
    tree.insert(7);
    tree.insert(9);
    tree.insert(2);
    tree.insert(6);
    assertRootState(tree, 4);
    assertInorder(tree, range(1, 9));

    assert(tree.remove(4));
    assertRootState(tree, 4);
    assertInorder(tree, {1, 2, 3, 5, 6, 7, 8, 9});

    assert(tree.remove(2));
    assertRootState(tree, 4);
    assertInorder(tree, {1, 3, 5, 6, 7, 8, 9});

    assert(tree.remove(1));
    assertRootState(tree, 3);
    assertInorder(tree, {3, 5, 6, 7, 8, 9});
}

void testSingleDeletionRotatesAtMultipleAncestors() {
    AVLTree tree;
    for (int key : {1, 10, 5, 7, 2, 6, 11, 8, 3, 12, 4, 9}) {
        tree.insert(key);
        tree.takeRotationEvents();
    }

    assert(tree.remove(1));
    const std::vector<AVLRotationEvent> events = tree.takeRotationEvents();
    assert(events.size() == 2);
    assert(events[0].type == AVLRotationType::Left);
    assert(events[0].mutation == AVLMutationType::Delete);
    assert(events[0].pivotKey == 2);
    assert(events[0].operationKey == 1);
    assert(events[1].type == AVLRotationType::RightLeft);
    assert(events[1].mutation == AVLMutationType::Delete);
    assert(events[1].pivotKey == 5);
    assert(events[1].operationKey == 1);
    assertInorder(tree, {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
}

void testDeletionUntilEmpty() {
    // Delete every key of a larger tree in a mixed order; the tree must
    // stay balanced throughout and end up empty.
    AVLTree tree;
    for (int key = 1; key <= 100; ++key) {
        tree.insert((key * 37) % 101);
    }
    assertHeightBounds(tree, 100);

    for (int key = 1; key <= 100; ++key) {
        assert(tree.remove(key));
        if (key < 100) {
            assertHeightBounds(tree, 100 - key);
            assert(!tree.search(key));
        }
    }
    assertRootState(tree, 0);
    assertInorder(tree, {});
    for (int key = 1; key <= 100; ++key) {
        assert(!tree.search(key));
    }
}

} // namespace

int main() {
    bool assertionsActive = false;
    assert((assertionsActive = true));
    if (!assertionsActive) {
        std::fprintf(stderr, "FAIL: assertions are disabled; rebuild tests with checks enabled.\n");
        return 1;
    }

    testOwnershipContract();
    testSnapshot();
    testInsertionAndSearch();
    testTraversalOrdersAndEmptyTree();
    testRecursiveInvariantsAfterMutations();
    testInsertionRotations();
    testDeletionStructuralCases();
    testDeletionTriggeredSingleRotation();
    testDeletionTriggeredRightRotation();
    testDeletionTriggeredDoubleRotation();
    testDeletionRebalancesMultipleAncestors();
    testSingleDeletionRotatesAtMultipleAncestors();
    testDeletionUntilEmpty();
    return 0;
}
