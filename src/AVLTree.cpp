#include "AVLTree.h"

AVLTree::AVLTree() noexcept : root_(nullptr) {}

AVLTree::~AVLTree() noexcept {
    clear();
}

int AVLTree::nodeHeight(const AVLNode* node) noexcept {
    return node == nullptr ? 0 : node->height;
}

int AVLTree::nodeBalanceFactor(const AVLNode* node) noexcept {
    return node == nullptr
        ? 0
        : nodeHeight(node->left) - nodeHeight(node->right);
}

void AVLTree::updateHeight(AVLNode* node) noexcept {
    if (node != nullptr) {
        const int leftHeight = nodeHeight(node->left);
        const int rightHeight = nodeHeight(node->right);
        node->height = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
    }
}

int AVLTree::getHeight() const noexcept {
    return nodeHeight(root_);
}

int AVLTree::getBalanceFactor() const noexcept {
    return nodeBalanceFactor(root_);
}

void AVLTree::clear() noexcept {
    destroySubtree(root_);
    root_ = nullptr;
}

void AVLTree::destroySubtree(AVLNode* node) noexcept {
    if (node != nullptr) {
        destroySubtree(node->left);
        destroySubtree(node->right);
        delete node;
    }
}
