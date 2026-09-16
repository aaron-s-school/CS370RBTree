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
static struct rbnode *find_node_by_key(const rbtree_t *t, const char *key) {
    struct rbnode *cur = t->root;

    /* invariant: cur is the still-unsearched subtree that may hold key */
    while (cur != NULL) {
        int cmp = strcmp(key, cur->key);
        if (cmp == 0) {
            return cur;
        }
        cur = (cmp < 0) ? cur->left : cur->right;
    }
    return NULL;
}

void *rb_find(const rbtree_t *t, const char *key) {
    struct rbnode *node = find_node_by_key(t, key);
    return (node == NULL) ? NULL : node->value;
}

/* Precondition: subtree_root != NULL. */
static struct rbnode *leftmost_node(struct rbnode *subtree_root) {
    struct rbnode *cur = subtree_root;

    /* invariant: every node walked past so far had a left child, so the true
     * leftmost node is still reachable from cur */
    while (cur->left != NULL) {
        cur = cur->left;
    }
    return cur;
}

/* Relinks new_subtree into old_subtree's place under old_subtree's parent
 * (or as t->root). new_subtree may be NULL; old_subtree may not. Does not
 * touch old_subtree's own child/parent pointers. */
static void replace_in_parent(rbtree_t *t, struct rbnode *old_subtree, struct rbnode *new_subtree) {
    if (old_subtree->parent == NULL) {
        t->root = new_subtree;
    } else if (old_subtree == old_subtree->parent->left) {
        old_subtree->parent->left = new_subtree;
    } else {
        old_subtree->parent->right = new_subtree;
    }

    if (new_subtree != NULL) {
        new_subtree->parent = old_subtree->parent;
    }
}

static rb_color_t effective_color(const struct rbnode *node) {
    return (node == NULL) ? RB_BLACK : node->color;
}

/* doubly_black_node may be NULL (a black NULL "leaf" going double-black),
 * which is why its parent is passed explicitly rather than read off it. */
static void rb_delete_fixup(rbtree_t *t, struct rbnode *doubly_black_node, struct rbnode *doubly_black_parent) {
    /* invariant: doubly_black_node carries one extra unit of black-height its
     * subtree is missing; doubly_black_parent is tracked explicitly because
     * doubly_black_node may be NULL. sibling is never NULL here -- if it
     * were, its side would have less black-height than doubly_black_node's
     * side, contradicting that the tree was valid before deletion. */
    while (doubly_black_node != t->root && effective_color(doubly_black_node) == RB_BLACK) {
        if (doubly_black_node == doubly_black_parent->left) {
            struct rbnode *sibling = doubly_black_parent->right;

            if (sibling->color == RB_RED) {
                /* case 1: red sibling -> rotate so the new sibling is black, then fall through */
                rb_node_recolor_black(sibling);
                rb_node_recolor_red(doubly_black_parent);
                rotate_left(t, doubly_black_parent);
                sibling = doubly_black_parent->right;
            }

            if (effective_color(sibling->left) == RB_BLACK && effective_color(sibling->right) == RB_BLACK) {
                /* case 2: both nephews black -> recolor sibling red, push the double-black up */
                rb_node_recolor_red(sibling);
                doubly_black_node = doubly_black_parent;
                doubly_black_parent = doubly_black_parent->parent;
            } else {
                if (effective_color(sibling->right) == RB_BLACK) {
                    /* case 3: near nephew red, far nephew black -> rotate to convert to case 4 */
                    rb_node_recolor_black(sibling->left);
                    rb_node_recolor_red(sibling);
                    rotate_right(t, sibling);
                    sibling = doubly_black_parent->right;
                }
                /* case 4: far nephew red -> recolor and rotate; terminates the loop */
                sibling->color = doubly_black_parent->color;
                rb_node_recolor_black(doubly_black_parent);
                rb_node_recolor_black(sibling->right);
                rotate_left(t, doubly_black_parent);
                doubly_black_node = t->root;
            }
        } else {
            /* mirror of the above with left/right swapped */
            struct rbnode *sibling = doubly_black_parent->left;

            if (sibling->color == RB_RED) {
                rb_node_recolor_black(sibling);
                rb_node_recolor_red(doubly_black_parent);
                rotate_right(t, doubly_black_parent);
                sibling = doubly_black_parent->left;
            }

            if (effective_color(sibling->right) == RB_BLACK && effective_color(sibling->left) == RB_BLACK) {
                rb_node_recolor_red(sibling);
                doubly_black_node = doubly_black_parent;
                doubly_black_parent = doubly_black_parent->parent;
            } else {
                if (effective_color(sibling->left) == RB_BLACK) {
                    rb_node_recolor_black(sibling->right);
                    rb_node_recolor_red(sibling);
                    rotate_left(t, sibling);
                    sibling = doubly_black_parent->left;
                }
                sibling->color = doubly_black_parent->color;
                rb_node_recolor_black(doubly_black_parent);
                rb_node_recolor_black(sibling->left);
                rotate_right(t, doubly_black_parent);
                doubly_black_node = t->root;
            }
        }
    }

    rb_node_recolor_black(doubly_black_node);
}

int rb_delete(rbtree_t *t, const char *key) {
    struct rbnode *node_to_del = find_node_by_key(t, key);
    if (node_to_del == NULL) {
        return -1;
    }

    struct rbnode *spliced_node = node_to_del;
    rb_color_t spliced_node_original_color = spliced_node->color;
    struct rbnode *replacement_node;
    struct rbnode *replacement_parent;

    if (node_to_del->left == NULL) {
        replacement_node = node_to_del->right;
        replacement_parent = node_to_del->parent;
        replace_in_parent(t, node_to_del, node_to_del->right);
    } else if (node_to_del->right == NULL) {
        replacement_node = node_to_del->left;
        replacement_parent = node_to_del->parent;
        replace_in_parent(t, node_to_del, node_to_del->left);
    } else {
        spliced_node = leftmost_node(node_to_del->right);
        spliced_node_original_color = spliced_node->color;
        replacement_node = spliced_node->right;

        if (spliced_node->parent == node_to_del) {
            replacement_parent = spliced_node;
        } else {
            replacement_parent = spliced_node->parent;
            replace_in_parent(t, spliced_node, spliced_node->right);
            spliced_node->right = node_to_del->right;
            spliced_node->right->parent = spliced_node;
        }

        replace_in_parent(t, node_to_del, spliced_node);
        spliced_node->left = node_to_del->left;
        spliced_node->left->parent = spliced_node;
        spliced_node->color = node_to_del->color;
    }

    free(node_to_del->key);
    if (t->value_free != NULL) {
        t->value_free(node_to_del->value);
    }
    free(node_to_del);
    t->size--;

    if (spliced_node_original_color == RB_BLACK) {
        rb_delete_fixup(t, replacement_node, replacement_parent);
    }

    return 0;
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
/* Returns the black-height of the subtree rooted at node (number of black
 * nodes on any root-to-NULL path, not counting node itself), or -1 if the
 * subtree already violates the equal-black-height invariant. */
static int black_height(const struct rbnode *node) {
    if (node == NULL) {
        return 0;
    }
    int left_bh = black_height(node->left);
    if (left_bh < 0) {
        return -1;
    }
    int right_bh = black_height(node->right);
    if (right_bh < 0) {
        return -1;
    }
    if (left_bh != right_bh) {
        return -1;
    }
    return left_bh + (node->color == RB_BLACK ? 1 : 0);
}

int rb_validate(const rbtree_t *t) {
    if (t->root == NULL) {
        return 0;
    }
    if (t->root->color == RB_RED) {
        return -1;
    }
    if (black_height(t->root) < 0) {
        return -1;
    }

    struct rbnode **stack = malloc(t->size * sizeof *stack);
    if (stack == NULL) {
        return -1;
    }

    size_t top = 0;
    struct rbnode *cur = t->root;
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
        if (cur->color == RB_RED && cur->parent != NULL && cur->parent->color == RB_RED) {
            result = -1;
            break;
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
