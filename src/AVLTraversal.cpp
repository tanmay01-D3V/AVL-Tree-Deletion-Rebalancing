#include "AVLTree.h"

// Depth-first traversals. An empty tree yields an empty vector.

void AVLTree::inorder(const AVLNode* node, std::vector<int>& values) {
    if (node == nullptr) {
        return;
    }
    inorder(node->left, values);
    values.push_back(node->key);
    inorder(node->right, values);
}

void AVLTree::preorder(const AVLNode* node, std::vector<int>& values) {
    if (node == nullptr) {
        return;
    }
    values.push_back(node->key);
    preorder(node->left, values);
    preorder(node->right, values);
}

void AVLTree::postorder(const AVLNode* node, std::vector<int>& values) {
    if (node == nullptr) {
        return;
    }
    postorder(node->left, values);
    postorder(node->right, values);
    values.push_back(node->key);
}

std::vector<int> AVLTree::inorderTraversal() const {
    std::vector<int> values;
    inorder(root_, values);
    return values;
}

std::vector<int> AVLTree::preorderTraversal() const {
    std::vector<int> values;
    preorder(root_, values);
    return values;
}

std::vector<int> AVLTree::postorderTraversal() const {
    std::vector<int> values;
    postorder(root_, values);
    return values;
}
