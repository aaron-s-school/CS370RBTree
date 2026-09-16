#include <rbtree.h>
#include <stdlib.h>
#include <string.h>

typedef enum { RB_RED, RB_BLACK } rb_color_t;

struct rbnode {
    char          *key;
    void          *value;
    rb_color_t     color;
    struct rbnode *left;
    struct rbnode *right;
    struct rbnode *parent;
};

struct rbtree {
    struct rbnode   *root;
    size_t           size;
    rb_value_free_fn value_free;
};

rbtree_t *rb_create(rb_value_free_fn value_free) {
    rbtree_t *t = malloc(sizeof *t);
    if (t == NULL) {
        return NULL;
    }
    t->root = NULL;
    t->size = 0;
    t->value_free = value_free;
    return t;
}
/* Precondition: x->right != NULL. Caller (future fixup) guarantees this. */
static void rotate_left(rbtree_t *t, struct rbnode *x) {
    struct rbnode *y = x->right;

    x->right = y->left;
    if (y->left != NULL) {
        y->left->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == NULL) {
        t->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }

    y->left = x;
    x->parent = y;
}

/* Precondition: x->left != NULL. Caller (future fixup) guarantees this. */
static void rotate_right(rbtree_t *t, struct rbnode *x) {
    struct rbnode *y = x->left;

    x->left = y->right;
    if (y->right != NULL) {
        y->right->parent = x;
    }

    y->parent = x->parent;
    if (x->parent == NULL) {
        t->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }

    y->right = x;
    x->parent = y;
}
void rb_node_recolor_black(struct rbnode *node_to_color){
    if(node_to_color == NULL){
        return;
    }
    node_to_color->color = RB_BLACK;

}
void rb_node_recolor_red(struct rbnode *node_to_color){
    if(node_to_color == NULL){
        return;
    }
    node_to_color->color = RB_RED;

}

void rb_insert_fixup(rbtree_t *t, struct rbnode *inserted_node) {
    struct rbnode *z = inserted_node;

    /* invariant: the only possible violation is z red with z->parent red;
     * root stays black at the top of every iteration, so z->parent red
     * implies z->parent is not root and grandparent is non-NULL */
    while (z->parent != NULL && z->parent->color == RB_RED) {
        struct rbnode *parent = z->parent;
        struct rbnode *grandparent = parent->parent;

        if (parent == grandparent->left) {
            struct rbnode *uncle = grandparent->right;
            if (uncle != NULL && uncle->color == RB_RED) {
                rb_node_recolor_black(parent);
                rb_node_recolor_black(uncle);
                rb_node_recolor_red(grandparent);
                z = grandparent;
            } else {
                if (z == parent->right) {
                    z = parent;
                    rotate_left(t, z);
                }
                /* z's parent/grandparent may have changed by the rotation above */
                parent = z->parent;
                grandparent = parent->parent;
                rb_node_recolor_black(parent);
                rb_node_recolor_red(grandparent);
                rotate_right(t, grandparent);
            }
        } else {
            struct rbnode *uncle = grandparent->left;
            if (uncle != NULL && uncle->color == RB_RED) {
                rb_node_recolor_black(parent);
                rb_node_recolor_black(uncle);
                rb_node_recolor_red(grandparent);
                z = grandparent;
            } else {
                if (z == parent->left) {
                    z = parent;
                    rotate_right(t, z);
                }
                parent = z->parent;
                grandparent = parent->parent;
                rb_node_recolor_black(parent);
                rb_node_recolor_red(grandparent);
                rotate_left(t, grandparent);
            }
        }
    }

    rb_node_recolor_black(t->root);
}


int rb_insert(rbtree_t *t, const char *key, void *value) {
    struct rbnode *parent = NULL;
    struct rbnode *cur = t->root;
    int cmp = 0;

    size_t key_len = strlen(key) + 1;
    char *key_copy = malloc(key_len);
    if (key_copy == NULL) {
        return -1;
    }
    memcpy(key_copy, key, key_len);
    
    struct rbnode *node = malloc(sizeof *node);
    if (node == NULL) {
        goto fail_node;
    }

    node->key = key_copy;
    node->value = value;
    node->color = RB_RED;
    node->left = NULL;
    node->right = NULL;
    
    
    if (cur == NULL) {
        node->parent = NULL;
        t->root = node;
        node->color = RB_BLACK;
        t->size++;
        return 0;

    }

    /* invariant: cur is the still-unsearched subtree; parent trails it */
    while (cur != NULL) {
        cmp = strcmp(key, cur->key);
        if (cmp == 0) {
            if (t->value_free != NULL) {
                t->value_free(cur->value);
            }
            cur->value = value;
            free(key_copy);
            free(node);
            return 0;
        }
        parent = cur;
        cur = (cmp < 0) ? cur->left : cur->right;
    }

    node->parent = parent;

    

     if (cmp < 0) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    t->size++;
    rb_insert_fixup(t,node);
    return 0;

fail_node:
    free(key_copy);
    return -1;
}
void *rb_find(const rbtree_t *t, const char *key) {
    struct rbnode *cur = t->root;

    /* invariant: cur is the still-unsearched subtree that may hold key */
    while (cur != NULL) {
        int cmp = strcmp(key, cur->key);
        if (cmp == 0) {
            return cur->value;
        }
        cur = (cmp < 0) ? cur->left : cur->right;
    }
    return NULL;
}
int rb_delete(rbtree_t *t, const char *key){
    (void)t;
    (void)key;
    return -1;
}
size_t rb_size(const rbtree_t *t){
    return t->size;

}
static void foreach_inorder(const struct rbnode *node,
                             void (*fn)(const char *key, void *value, void *ctx),
                             void *ctx) {
    if (node == NULL) {
        return;
    }
    /* in-order: left subtree, then this node, then right subtree, so fn
     * sees keys in ascending sorted order */
    foreach_inorder(node->left, fn, ctx);
    fn(node->key, node->value, ctx);
    foreach_inorder(node->right, fn, ctx);
}

void rb_foreach(const rbtree_t *t,
void (*fn)(const char *key, void *value, void *ctx),
void *ctx){
    foreach_inorder(t->root, fn, ctx);
}
/* NOTE: only checks BST key ordering; root-color, and
 * black-height invariants are not yet checked */
int rb_validate(const rbtree_t *t) {
    //TODO check black height
    if (t->root == NULL) {
        return 0;
    }

    struct rbnode **stack = malloc(t->size * sizeof *stack);
    if (stack == NULL) {
        return -1;
    }

    size_t top = 0;
    struct rbnode *cur = t->root;
    if(cur->color == RB_RED){
        return -1;
    }
    const char *prev_key = NULL;
    int result = 0;

    /* invariant: stack holds ancestors on the current path still awaiting their in-order visit */
    while (cur != NULL || top > 0) {
        while (cur != NULL) {
            stack[top++] = cur;
            cur = cur->left;
        }
        cur = stack[--top];
        if (prev_key != NULL && strcmp(prev_key, cur->key) >= 0) {
            result = -1;
            break;
        }
        if(cur->color == RB_RED && cur->parent !=NULL){
            if(cur->parent->color == RB_RED){
                return -1;
            }
        }
        prev_key = cur->key;
        cur = cur->right;
    }

    free(stack);
    return result;
}
static void free_subtree(struct rbnode *node, rb_value_free_fn value_free) {
    if (node == NULL) {
        return;
    }
    /* post-order: free children before the parent so we don't lose the pointers to reach them */
    free_subtree(node->left, value_free);
    free_subtree(node->right, value_free);
    free(node->key);
    if (value_free != NULL) {
        value_free(node->value);
    }
    free(node);
}

void rb_destroy(rbtree_t *t) {
    if (t == NULL) {
        return;
    }
    free_subtree(t->root, t->value_free);
    free(t);
}
