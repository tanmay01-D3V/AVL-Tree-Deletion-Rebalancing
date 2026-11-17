#include "AVLTree.h"

// Insertion and search.
//
// Duplicates are ignored (documented policy). While the recursion unwinds,
// each ancestor is passed through the shared rebalance() hook, which updates
// heights and applies any rotation the new key triggers.

AVLNode* AVLTree::insertNode(AVLNode* node, int key) {
    if (node == nullptr) {
        return new AVLNode(key);
    }

    if (key < node->key) {
        node->left = insertNode(node->left, key);
    } else if (key > node->key) {
        node->right = insertNode(node->right, key);
    } else {
        return node; // Duplicate key: ignore.
    }

    return rebalance(node);
}

bool AVLTree::containsNode(const AVLNode* node, int key) noexcept {
    while (node != nullptr) {
        if (key < node->key) {
            node = node->left;
        } else if (key > node->key) {
            node = node->right;
        } else {
            return true;
        }
    }
    return false;
}

void AVLTree::insert(int key) {
    rotationEvents_.clear();
    mutationType_ = AVLMutationType::Insert;
    operationKey_ = key;
    root_ = insertNode(root_, key);
}

bool AVLTree::search(int key) const noexcept {
    return containsNode(root_, key);
}
