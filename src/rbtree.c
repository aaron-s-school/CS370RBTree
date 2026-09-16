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
    int              black_height;
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
    t->black_height = 0;
    return t;
}
int rb_insert(rbtree_t *t, const char *key, void *value) {
    struct rbnode *parent = NULL;
    struct rbnode *cur = t->root;
    int cmp = 0;

    /* invariant: cur is the still-unsearched subtree; parent trails it */
    while (cur != NULL) {
        cmp = strcmp(key, cur->key);
        if (cmp == 0) {
            if (t->value_free != NULL) {
                t->value_free(cur->value);
            }
            cur->value = value;
            return 0;
        }
        parent = cur;
        cur = (cmp < 0) ? cur->left : cur->right;
    }

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
    node->parent = parent;

    if (parent == NULL) {
        t->root = node;
        node->color = RB_BLACK;
        t->black_height++;
    } else if (cmp < 0) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    t->size++;
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
void rb_foreach(const rbtree_t *t,
void (*fn)(const char *key, void *value, void *ctx),
void *ctx){
    (void)t;
    (void)fn;
    (void)ctx;
    return;
}
/* NOTE: only checks BST key ordering; root-color, red-red, and
 * black-height invariants are not yet checked (rb_insert has no
 * rebalancing yet, so those checks aren't meaningful until it does). */
int rb_validate(const rbtree_t *t) {
    if (t->root == NULL) {
        return 0;
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
