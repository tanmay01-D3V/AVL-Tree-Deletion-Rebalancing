#include "AVLNode.h"
#include "AVLTree.h"

#include <cassert>
#include <type_traits>

int main() {
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
