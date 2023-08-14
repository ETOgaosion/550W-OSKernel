#pragma once

#define BLACK 0
#define RED 1

typedef int KeyType;
typedef void * ValueType;

typedef struct RBTreeNode {
    unsigned char color;
    KeyType key;
    ValueType value;
    struct RBTreeNode *left;
    struct RBTreeNode *right;
    struct RBTreeNode *parent;
} RBNode, *RBTree;

typedef struct rb_root {
    RBNode *node;
} RBRoot;

// 创建红黑树，返回"红黑树的根"！
void k_rbroot_init(RBRoot *root);
void k_rbnode_init(RBNode *node, KeyType key, ValueType value);

void k_rbtree_destroy(RBRoot *root);

int k_rbtree_insert(RBRoot *root, RBNode *node, KeyType key);

void k_rbtree_delete(RBRoot *root, RBNode *node);

void k_rbroot_free(RBNode *node);

int k_rbtree_search(RBRoot *root, KeyType key);
int k_rbtree_iterative_search(RBRoot *root, KeyType key);

RBNode *k_rbtree_minimum(RBRoot *root);
RBNode *k_rbtree_maximum(RBRoot *root);

// 打印红黑树
void k_rbtree_print(RBRoot *root);