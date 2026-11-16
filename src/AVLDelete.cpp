#include "AVLTree.h"

// Deletion with full upward rebalancing.
//
// removeNode() returns the subtree that replaces `node`. While the recursion
// unwinds, EVERY ancestor on the search path is height-updated and passed
// through rebalance(), so a deletion that unbalances several ancestors is
// fully repaired, not just the first unbalanced node. A key that is not
// present leaves the tree untouched and `removed` stays false.

AVLNode* AVLTree::removeNode(AVLNode* node, int key, bool& removed) {
    if (node == nullptr) {
        return nullptr; // Key not present: nothing changes.
    }

    if (key < node->key) {
        node->left = removeNode(node->left, key, removed);
    } else if (key > node->key) {
        node->right = removeNode(node->right, key, removed);
    } else {
        removed = true;

        if (node->left == nullptr || node->right == nullptr) {
            // Leaf or one-child case: the single surviving child (or null)
            // takes this node's place. The parent level then rebalances.
            AVLNode* replacement = node->left != nullptr ? node->left : node->right;
            delete node;
            return replacement;
        }

        // Two-children case: copy the inorder successor's key here, then
        // delete the successor from the right subtree. The successor has no
        // left child, so that recursive call resolves as a leaf/one-child
        // deletion. `removed` is already true, so a separate flag is used.
        AVLNode* successor = node->right;
        while (successor->left != nullptr) {
            successor = successor->left;
        }
        node->key = successor->key;
        bool successorRemoved = false;
        node->right = removeNode(node->right, successor->key, successorRemoved);
    }

    // Every affected ancestor is height-updated and rebalanced here,
    // including cases where more than one ancestor must rotate.
    return rebalance(node);
}

bool AVLTree::remove(int key) {
    bool removed = false;
    root_ = removeNode(root_, key, removed);
    return removed;
}
