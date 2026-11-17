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
    rotationEvents_.clear();
    mutationType_ = AVLMutationType::Insert;
    operationKey_ = 0;
}

std::vector<AVLTreeNodeSnapshot> AVLTree::snapshot() const {
    std::vector<AVLTreeNodeSnapshot> nodes;
    appendSnapshot(root_, nodes);
    return nodes;
}

std::vector<AVLRotationEvent> AVLTree::takeRotationEvents() {
    std::vector<AVLRotationEvent> events;
    events.swap(rotationEvents_);
    return events;
}

int AVLTree::appendSnapshot(const AVLNode* node,
                            std::vector<AVLTreeNodeSnapshot>& nodes) {
    if (node == nullptr) {
        return -1;
    }

    const int index = static_cast<int>(nodes.size());
    nodes.push_back({node->key, -1, -1, node->height, nodeBalanceFactor(node)});
    nodes[static_cast<std::size_t>(index)].leftIndex =
        appendSnapshot(node->left, nodes);
    nodes[static_cast<std::size_t>(index)].rightIndex =
        appendSnapshot(node->right, nodes);
    return index;
}

void AVLTree::destroySubtree(AVLNode* node) noexcept {
    if (node != nullptr) {
        destroySubtree(node->left);
        destroySubtree(node->right);
        delete node;
    }
}
