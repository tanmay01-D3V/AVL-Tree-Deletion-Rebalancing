#ifndef AVL_TREE_H
#define AVL_TREE_H

#include "AVLNode.h"

#include <vector>

class AVLTree {
public:
    AVLTree() noexcept;
    ~AVLTree() noexcept;

    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    void insert(int key);
    bool remove(int key);
    bool search(int key) const noexcept;

    std::vector<int> inorderTraversal() const;
    std::vector<int> preorderTraversal() const;
    std::vector<int> postorderTraversal() const;

    int getHeight() const noexcept;
    int getBalanceFactor() const noexcept;
    void clear() noexcept;

private:
    AVLNode* root_;

    static int nodeHeight(const AVLNode* node) noexcept;
    static int nodeBalanceFactor(const AVLNode* node) noexcept;
    static void updateHeight(AVLNode* node) noexcept;

    static AVLNode* rotateLeft(AVLNode* node) noexcept;
    static AVLNode* rotateRight(AVLNode* node) noexcept;
    static AVLNode* rebalance(AVLNode* node) noexcept;

    static AVLNode* insertNode(AVLNode* node, int key);
    static AVLNode* removeNode(AVLNode* node, int key, bool& removed);
    static bool containsNode(const AVLNode* node, int key) noexcept;

    static void inorder(const AVLNode* node, std::vector<int>& values);
    static void preorder(const AVLNode* node, std::vector<int>& values);
    static void postorder(const AVLNode* node, std::vector<int>& values);

    static void destroySubtree(AVLNode* node) noexcept;
};

#endif
