#include "tree.h"

// Рекурсивное удаление поддерева в пост-порядке (сначала дети, потом узел).
static void destroy(TNode* node) {
    if (node) { destroy(node->left); destroy(node->right); delete node; }
}

// Обход в ширину (BFS): возвращает узлы в порядке уровней (слева направо).
std::vector<TNode*> treeNodes(const CBTree& tree) {
    std::vector<TNode*> nodes;
    if (tree.root) nodes.push_back(tree.root);
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i]->left) nodes.push_back(nodes[i]->left);
        if (nodes[i]->right) nodes.push_back(nodes[i]->right);
    }
    return nodes;
}

// Вставка с сохранением ПОЛНОТЫ дерева: новый узел должен занять
// первое свободное место в порядке уровней — либо пустой left,
// либо пустой right у первого подходящего узла.
void treeInsert(CBTree& tree, const std::string& key) {
    if (!tree.root) { tree.root = new TNode{key, nullptr, nullptr}; return; }
    std::vector<TNode*> nodes = treeNodes(tree);
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (!nodes[i]->left) { nodes[i]->left = new TNode{key, nullptr, nullptr}; return; }
        if (!nodes[i]->right) { nodes[i]->right = new TNode{key, nullptr, nullptr}; return; }
    }
}

// BFS. Сложность O(n).
bool treeContains(const CBTree& tree, const std::string& key) {
    std::vector<TNode*> nodes = treeNodes(tree);
    for (std::size_t i = 0; i < nodes.size(); ++i) if (nodes[i]->key == key) return true;
    return false;
}

// Полная очистка: удаляем все узлы и обнуляем корень.
void treeClear(CBTree& tree) { destroy(tree.root); tree.root = nullptr; }