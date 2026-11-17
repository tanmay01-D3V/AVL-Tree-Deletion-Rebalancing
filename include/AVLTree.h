#ifndef AVL_TREE_H
#define AVL_TREE_H

#include "AVLNode.h"

#include <vector>

enum class AVLRotationType {
    Left,
    Right,
    LeftRight,
    RightLeft
};

enum class AVLMutationType {
    Insert,
    Delete
};

struct AVLRotationEvent {
    AVLRotationType type;
    AVLMutationType mutation;
    int pivotKey;
    int operationKey;
};

struct AVLTreeNodeSnapshot {
    int key;
    int leftIndex;
    int rightIndex;
    int height;
    int balanceFactor;
};

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
    std::vector<AVLTreeNodeSnapshot> snapshot() const;
    std::vector<AVLRotationEvent> takeRotationEvents();

    int getHeight() const noexcept;
    int getBalanceFactor() const noexcept;
    void clear() noexcept;

private:
    friend struct AVLTreeInvariantTestAccess;

    AVLNode* root_;
    std::vector<AVLRotationEvent> rotationEvents_;
    AVLMutationType mutationType_ = AVLMutationType::Insert;
    int operationKey_ = 0;

    static int nodeHeight(const AVLNode* node) noexcept;
    static int nodeBalanceFactor(const AVLNode* node) noexcept;
    static void updateHeight(AVLNode* node) noexcept;

    static AVLNode* rotateLeft(AVLNode* node) noexcept;
    static AVLNode* rotateRight(AVLNode* node) noexcept;
    AVLNode* rebalance(AVLNode* node);

    AVLNode* insertNode(AVLNode* node, int key);
    AVLNode* removeNode(AVLNode* node, int key, bool& removed);
    static bool containsNode(const AVLNode* node, int key) noexcept;

    static void inorder(const AVLNode* node, std::vector<int>& values);
    static void preorder(const AVLNode* node, std::vector<int>& values);
    static void postorder(const AVLNode* node, std::vector<int>& values);
    static int appendSnapshot(const AVLNode* node,
                              std::vector<AVLTreeNodeSnapshot>& nodes);

    static void destroySubtree(AVLNode* node) noexcept;
};

#endif
