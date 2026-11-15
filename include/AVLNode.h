#ifndef AVL_NODE_H
#define AVL_NODE_H

struct AVLNode {
    int key;
    AVLNode* left;
    AVLNode* right;
    int height;

    explicit AVLNode(int keyValue) noexcept
        : key(keyValue), left(nullptr), right(nullptr), height(1) {}
};

#endif
