#include "AVLTree.h"

// Shared balancing primitives.
//
// Conventions (see README): height(nullptr) == 0, a leaf has height 1, and
// balance factor == height(left) - height(right), so a valid AVL node keeps a
// balance factor in [-1, 1]. rebalance() restores that invariant for the LL,
// RR, LR, and RL cases and is the single balancing hook used by insertion and
// deletion.

AVLNode* AVLTree::rotateLeft(AVLNode* node) noexcept {
    AVLNode* pivot = node->right;
    node->right = pivot->left;
    pivot->left = node;
    updateHeight(node);
    updateHeight(pivot);
    return pivot;
}

AVLNode* AVLTree::rotateRight(AVLNode* node) noexcept {
    AVLNode* pivot = node->left;
    node->left = pivot->right;
    pivot->right = node;
    updateHeight(node);
    updateHeight(pivot);
    return pivot;
}

AVLNode* AVLTree::rebalance(AVLNode* node) noexcept {
    if (node == nullptr) {
        return nullptr;
    }

    updateHeight(node);
    const int balance = nodeBalanceFactor(node);

    if (balance > 1) {
        // Left-heavy: LR case when the left child itself leans right,
        // otherwise a single right rotation (covers a left child whose
        // balance factor is +1 or 0, the latter arising during deletion).
        if (nodeBalanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
        }
        return rotateRight(node);
    }

    if (balance < -1) {
        // Right-heavy: RL case when the right child itself leans left,
        // otherwise a single left rotation (covers a right child whose
        // balance factor is -1 or 0, the latter arising during deletion).
        if (nodeBalanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
        }
        return rotateLeft(node);
    }

    return node;
}
