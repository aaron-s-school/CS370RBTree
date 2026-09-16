#include <rbtree.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO make check be against ordered array vals of similar operations
// TODO tests for delete and for each
static int failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        failures++; \
    } \
} while (0)

static void test_create_destroy_empty(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);
    CHECK(rb_size(t) == 0);
    rb_destroy(t);
    rb_destroy(NULL);
}

static void test_insert_find_basic(void) {
    rbtree_t *t = rb_create(free);
    CHECK(t != NULL);

    const char *keys[] = { "banana", "apple", "cherry", "date" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted with value i */
    for (size_t i = 0; i < n; i++) {
        int *v = malloc(sizeof *v);
        CHECK(v != NULL);
        *v = (int)i;
        CHECK(rb_insert(t, keys[i], v) == 0);
    }

    /* invariant: keys[0..i) have already been checked against their expected value */
    for (size_t i = 0; i < n; i++) {
        int *v = rb_find(t, keys[i]);
        CHECK(v != NULL);
        if (v != NULL) {
            CHECK(*v == (int)i);
        }
    }

    CHECK(rb_find(t, "missing") == NULL);
    rb_destroy(t);
}

static void test_insert_overwrite_frees_old_and_keeps_size(void) {
    rbtree_t *t = rb_create(free);
    CHECK(t != NULL);

    int *v1 = malloc(sizeof *v1);
    *v1 = 1;
    CHECK(rb_insert(t, "key", v1) == 0);
    CHECK(rb_size(t) == 1);

    int *v2 = malloc(sizeof *v2);
    *v2 = 2;
    CHECK(rb_insert(t, "key", v2) == 0);
    CHECK(rb_size(t) == 1);

    int *found = rb_find(t, "key");
    CHECK(found != NULL);
    if (found != NULL) {
        CHECK(*found == 2);
    }

    rb_destroy(t);
}

static int borrowed_value = 42;

static void test_insert_borrowed_values_not_owned(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);
    CHECK(rb_insert(t, "key", &borrowed_value) == 0);
    CHECK(rb_find(t, "key") == &borrowed_value);
    rb_destroy(t);
}

static void test_validate_bst_ordering(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "m", "f", "t", "a", "h", "p", "z", "c" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], NULL) == 0);
    }

    CHECK(rb_validate(t) == 0);
    rb_destroy(t);
}

static void test_size_tracks_inserts(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "one", "two", "three", "four", "five" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, so size == i */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], NULL) == 0);
        CHECK(rb_size(t) == i + 1);
    }

    CHECK(rb_insert(t, "one", NULL) == 0);
    CHECK(rb_size(t) == n);

    rb_destroy(t);
}

static void test_insert_fixup_line_ascending(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "10", "20", "30" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted and validated */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], NULL) == 0);
        CHECK(rb_validate(t) == 0);
    }
    CHECK(rb_size(t) == n);

    rb_destroy(t);
}

static void test_insert_fixup_line_descending(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "30", "20", "10" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted and validated */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], NULL) == 0);
        CHECK(rb_validate(t) == 0);
    }
    CHECK(rb_size(t) == n);

    rb_destroy(t);
}

static void test_insert_fixup_zigzag_left_right(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "30", "10", "20" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted and validated */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], NULL) == 0);
        CHECK(rb_validate(t) == 0);
    }
    CHECK(rb_size(t) == n);

    rb_destroy(t);
}

static void test_insert_fixup_zigzag_right_left(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "10", "30", "20" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted and validated */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], NULL) == 0);
        CHECK(rb_validate(t) == 0);
    }
    CHECK(rb_size(t) == n);

    rb_destroy(t);
}

static void test_insert_fixup_recolor_case(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* "5","3","8" builds a black root with two red children; inserting "1"
     * under the red "3" hits a red uncle ("8"), forcing the recolor-and-climb
     * case up to the root. */
    const char *keys[] = { "5", "3", "8", "1" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted and validated */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], NULL) == 0);
        CHECK(rb_validate(t) == 0);
    }
    CHECK(rb_size(t) == n);

    rb_destroy(t);
}

static void test_insert_fixup_cascading_recolor(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* Two-digit keys keep lexicographic order equal to numeric order. This
     * mixed build-then-fill-in sequence is long enough to force repeated and
     * cascading rebalances (recolors and rotations) across multiple levels,
     * not just a single fixup pass. */
    const char *keys[] = {
        "50", "25", "75", "10", "30", "60", "80",
        "15", "27", "35", "65", "45"
    };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted and validated */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], NULL) == 0);
        CHECK(rb_validate(t) == 0);
    }
    CHECK(rb_size(t) == n);

    rb_destroy(t);
}

static void test_insert_no_stale_root_after_left_rotation(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* Ascending "10","20","30" forces rotate_left to move "20" above the
     * original root "10". If a rotation ever forgot to update t->root,
     * "20"/"30" would become unreachable from the (stale) root -- and
     * rb_validate wouldn't necessarily notice, since it never cross-checks
     * the number of nodes it visits against rb_size(). The two inserts
     * after the rotation also confirm later rb_insert calls search from
     * the updated root, not a cached/stale one. */
    const char *keys[] = { "10", "20", "30", "05", "40" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }

    /* invariant: keys[0..i) have already been confirmed reachable from the (possibly rotated) root */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_find(t, keys[i]) == (void *)keys[i]);
    }
    CHECK(rb_size(t) == n);
    CHECK(rb_validate(t) == 0);

    rb_destroy(t);
}

static void test_insert_no_stale_root_after_right_rotation(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* Mirror of the left-rotation case: descending "30","20","10" forces
     * rotate_right to move "20" above the original root "30". */
    const char *keys[] = { "30", "20", "10", "40", "05" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }

    /* invariant: keys[0..i) have already been confirmed reachable from the (possibly rotated) root */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_find(t, keys[i]) == (void *)keys[i]);
    }
    CHECK(rb_size(t) == n);
    CHECK(rb_validate(t) == 0);

    rb_destroy(t);
}

typedef struct {
    const char *keys[16];
    void       *values[16];
    size_t      count;
} foreach_capture_t;

static void foreach_capture_cb(const char *key, void *value, void *ctx) {
    foreach_capture_t *cap = ctx;
    cap->keys[cap->count] = key;
    cap->values[cap->count] = value;
    cap->count++;
}

static void test_foreach_visits_in_sorted_order(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "m", "f", "t", "a", "h", "p", "z", "c" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], NULL) == 0);
    }

    foreach_capture_t cap = { .count = 0 };
    rb_foreach(t, foreach_capture_cb, &cap);

    CHECK(cap.count == n);
    /* invariant: cap.keys[0..i) are already known to be in strictly ascending order */
    for (size_t i = 1; i < cap.count; i++) {
        CHECK(strcmp(cap.keys[i - 1], cap.keys[i]) < 0);
    }

    rb_destroy(t);
}

static void test_foreach_values_match(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "m", "f", "t", "a", "h", "p", "z", "c" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }

    foreach_capture_t cap = { .count = 0 };
    rb_foreach(t, foreach_capture_cb, &cap);

    CHECK(cap.count == n);
    /* invariant: cap.keys/cap.values[0..i) are already confirmed paired correctly;
     * matched by content since cap.keys[i] is the tree's internal key copy,
     * not the same pointer as the original keys[] literal */
    for (size_t i = 0; i < cap.count; i++) {
        const char *original = NULL;
        for (size_t j = 0; j < n; j++) {
            if (strcmp(keys[j], cap.keys[i]) == 0) {
                original = keys[j];
                break;
            }
        }
        CHECK(original != NULL);
        CHECK(cap.values[i] == (void *)original);
    }

    rb_destroy(t);
}

static void test_foreach_empty_tree_calls_nothing(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    foreach_capture_t cap = { .count = 0 };
    rb_foreach(t, foreach_capture_cb, &cap);
    CHECK(cap.count == 0);

    rb_destroy(t);
}
static void test_delete_missing_key_empty_tree(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    CHECK(rb_delete(t, "missing") == -1);
    CHECK(rb_size(t) == 0);
    CHECK(rb_validate(t) == 0);

    rb_destroy(t);
}

static void test_delete_missing_key_nonempty_tree(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "m", "f", "t", "a", "h" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }

    CHECK(rb_delete(t, "missing") == -1);
    CHECK(rb_size(t) == n);
    CHECK(rb_validate(t) == 0);

    /* invariant: keys[0..i) have already been confirmed still present after the failed delete */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_find(t, keys[i]) == (void *)keys[i]);
    }

    rb_destroy(t);
}

static void test_delete_only_root(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *key = "solo";
    CHECK(rb_insert(t, key, (void *)key) == 0);
    CHECK(rb_delete(t, "solo") == 0);

    CHECK(rb_size(t) == 0);
    CHECK(rb_find(t, "solo") == NULL);
    CHECK(rb_validate(t) == 0);

    rb_destroy(t);
}

static void test_delete_leaf_red(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* "m" becomes the black root; "f" and "t" insert as its red children
     * and neither triggers a fixup (parent m is black), so "f" is a red
     * leaf whose removal needs no rebalancing at all. */
    const char *keys[] = { "m", "f", "t" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }
    CHECK(rb_validate(t) == 0);

    CHECK(rb_delete(t, "f") == 0);

    CHECK(rb_size(t) == n - 1);
    CHECK(rb_find(t, "f") == NULL);
    CHECK(rb_find(t, keys[0]) == (void *)keys[0]); /* m */
    CHECK(rb_find(t, keys[2]) == (void *)keys[2]); /* t */
    CHECK(rb_validate(t) == 0);

    rb_destroy(t);
}

static void test_delete_node_with_only_left_child(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* Inserting "m","f","t" leaves f/t as red leaves under black root m.
     * Inserting "a" (< f) then lands under f's red-uncle (t) recolor case,
     * which recolors f and t black and m red-then-back-to-black at the
     * root, leaving f black with only a red left child "a" and no right
     * child. */
    const char *keys[] = { "m", "f", "t", "a" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }
    CHECK(rb_validate(t) == 0);

    CHECK(rb_delete(t, "f") == 0);

    CHECK(rb_size(t) == n - 1);
    CHECK(rb_find(t, "f") == NULL);
    CHECK(rb_find(t, keys[0]) == (void *)keys[0]); /* m */
    CHECK(rb_find(t, keys[2]) == (void *)keys[2]); /* t */
    CHECK(rb_find(t, keys[3]) == (void *)keys[3]); /* a, standing in for f */
    CHECK(rb_validate(t) == 0);

    rb_destroy(t);
}

static void test_delete_node_with_only_right_child(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* Mirror of the only-left-child case: "f","a","m" builds black root f
     * with red leaves a/m, then inserting "t" (> m) hits the red-uncle
     * recolor case, leaving m black with only a red right child "t" and no
     * left child. */
    const char *keys[] = { "f", "a", "m", "t" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }
    CHECK(rb_validate(t) == 0);

    CHECK(rb_delete(t, "m") == 0);

    CHECK(rb_size(t) == n - 1);
    CHECK(rb_find(t, "m") == NULL);
    CHECK(rb_find(t, keys[0]) == (void *)keys[0]); /* f */
    CHECK(rb_find(t, keys[1]) == (void *)keys[1]); /* a */
    CHECK(rb_find(t, keys[3]) == (void *)keys[3]); /* t, standing in for m */
    CHECK(rb_validate(t) == 0);

    rb_destroy(t);
}

static void test_delete_node_with_two_children(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* "m","f","t" builds black root m with black children f/t; "a" then "h"
     * insert as f's red left and red right children (no fixup, since f is
     * black), giving f exactly two children. f's in-order successor is "h"
     * (leftmost of f's right subtree, which is just h itself), a red leaf,
     * so splicing f out needs no double-black fixup -- only the two-children
     * copy-up logic is exercised here. */
    const char *keys[] = { "m", "f", "t", "a", "h" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }
    CHECK(rb_validate(t) == 0);

    CHECK(rb_delete(t, "f") == 0);

    CHECK(rb_size(t) == n - 1);
    CHECK(rb_find(t, "f") == NULL);
    CHECK(rb_find(t, keys[4]) == (void *)keys[4]); /* "h", now standing in for f's slot, value intact */
    CHECK(rb_find(t, keys[0]) == (void *)keys[0]); /* m */
    CHECK(rb_find(t, keys[2]) == (void *)keys[2]); /* t */
    CHECK(rb_find(t, keys[3]) == (void *)keys[3]); /* a, unaffected */
    CHECK(rb_validate(t) == 0);

    /* in-order should read a, h, m, t -- confirms h's key correctly took
     * over f's position in sorted order, not just that it's findable */
    foreach_capture_t cap = { .count = 0 };
    rb_foreach(t, foreach_capture_cb, &cap);
    CHECK(cap.count == n - 1);
    if (cap.count == n - 1) {
        CHECK(strcmp(cap.keys[0], "a") == 0);
        CHECK(strcmp(cap.keys[1], "h") == 0);
        CHECK(strcmp(cap.keys[2], "m") == 0);
        CHECK(strcmp(cap.keys[3], "t") == 0);
    }

    rb_destroy(t);
}

static void test_delete_black_leaf_with_red_sibling_on_left(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* This insertion order builds: root "30"(B), right child "35"(B, leaf),
     * left child "10"(R) with its own children "20"(B, with red leaf
     * children "15"/"25") and "05"(B, leaf). Deleting "35" removes a black
     * leaf whose sibling "10" is red -- fixup case 1 (rotate to convert to a
     * black-sibling case), which then cascades into whichever of cases 2-4
     * applies to "10"'s children. */
    const char *keys[] = { "15", "35", "30", "10", "05", "20", "25" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }
    CHECK(rb_validate(t) == 0);

    CHECK(rb_delete(t, "35") == 0);

    CHECK(rb_size(t) == n - 1);
    CHECK(rb_find(t, "35") == NULL);
    CHECK(rb_validate(t) == 0);

    const char *expect_present[] = { "15", "30", "10", "05", "20", "25" };
    size_t np = sizeof expect_present / sizeof expect_present[0];
    /* invariant: expect_present[0..i) have already been confirmed present with their key pointer as value */
    for (size_t i = 0; i < np; i++) {
        CHECK(rb_find(t, expect_present[i]) == (void *)expect_present[i]);
    }

    rb_destroy(t);
}

static void test_delete_black_leaf_with_red_sibling_on_right(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* Mirror of the left-sibling case: this insertion order builds root
     * "10"(B), left child "05"(B, leaf), right child "25"(R) with its own
     * children "30"(B, leaf) and "15"(B, with red right leaf child "20", no
     * left). Deleting "05" removes a black leaf whose sibling "25" is red --
     * fixup case 1 mirrored, cascading into whichever of cases 2-4 (mirrored)
     * applies to "25"'s children. */
    const char *keys[] = { "10", "05", "15", "30", "25", "20" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }
    CHECK(rb_validate(t) == 0);

    CHECK(rb_delete(t, "05") == 0);

    CHECK(rb_size(t) == n - 1);
    CHECK(rb_find(t, "05") == NULL);
    CHECK(rb_validate(t) == 0);

    const char *expect_present[] = { "10", "15", "30", "25", "20" };
    size_t np = sizeof expect_present / sizeof expect_present[0];
    /* invariant: expect_present[0..i) have already been confirmed present with their key pointer as value */
    for (size_t i = 0; i < np; i++) {
        CHECK(rb_find(t, expect_present[i]) == (void *)expect_present[i]);
    }

    rb_destroy(t);
}

static int delete_free_call_count = 0;

static void counting_free(void *value) {
    delete_free_call_count++;
    free(value);
}

static void test_delete_frees_owned_value(void) {
    delete_free_call_count = 0;

    rbtree_t *t = rb_create(counting_free);
    CHECK(t != NULL);

    int *v = malloc(sizeof *v);
    CHECK(v != NULL);
    *v = 7;
    CHECK(rb_insert(t, "key", v) == 0);
    CHECK(delete_free_call_count == 0);

    CHECK(rb_delete(t, "key") == 0);
    CHECK(delete_free_call_count == 1);
    CHECK(rb_size(t) == 0);
    CHECK(rb_find(t, "key") == NULL);

    rb_destroy(t);
    CHECK(delete_free_call_count == 1); /* destroy must not free the already-deleted value again */
}

static void test_delete_does_not_free_borrowed_value(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    int local_value = 99;
    CHECK(rb_insert(t, "key", &local_value) == 0);
    CHECK(rb_delete(t, "key") == 0);

    CHECK(local_value == 99);
    CHECK(rb_size(t) == 0);
    CHECK(rb_find(t, "key") == NULL);

    rb_destroy(t);
    CHECK(local_value == 99);
}

static void test_delete_then_reinsert_same_key(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    const char *keys[] = { "m", "f", "t", "a", "h" };
    size_t n = sizeof keys / sizeof keys[0];

    /* invariant: keys[0..i) have already been inserted, each valued by its own key pointer */
    for (size_t i = 0; i < n; i++) {
        CHECK(rb_insert(t, keys[i], (void *)keys[i]) == 0);
    }
    CHECK(rb_validate(t) == 0);

    CHECK(rb_delete(t, "f") == 0);
    CHECK(rb_find(t, "f") == NULL);
    CHECK(rb_size(t) == n - 1);
    CHECK(rb_validate(t) == 0);

    int new_value = 1;
    CHECK(rb_insert(t, "f", &new_value) == 0);
    CHECK(rb_find(t, "f") == &new_value);
    CHECK(rb_size(t) == n);
    CHECK(rb_validate(t) == 0);

    /* invariant: keys[0..i) have already been confirmed still present, unaffected by the delete/reinsert of "f" */
    for (size_t i = 0; i < n; i++) {
        if (strcmp(keys[i], "f") == 0) {
            continue;
        }
        CHECK(rb_find(t, keys[i]) == (void *)keys[i]);
    }

    rb_destroy(t);
}

typedef enum { STRESS_OP_INSERT, STRESS_OP_DELETE } stress_op_type_t;

typedef struct {
    stress_op_type_t type;
    const char       *key;
} stress_op_t;

static void test_interleaved_insert_delete_stress(void) {
    rbtree_t *t = rb_create(NULL);
    CHECK(t != NULL);

    /* Two-digit keys keep lexicographic order equal to numeric order (see
     * insert_fixup_cascading_recolor). Every delete here targets a key
     * known to be present and every insert targets a key known to be
     * absent at that point in the script -- this exercises realistic
     * interleaving, not the already-covered missing-key/overwrite cases. */
    const stress_op_t ops[] = {
        { STRESS_OP_INSERT, "50" }, { STRESS_OP_INSERT, "25" }, { STRESS_OP_INSERT, "75" },
        { STRESS_OP_INSERT, "10" }, { STRESS_OP_INSERT, "30" }, { STRESS_OP_INSERT, "60" },
        { STRESS_OP_INSERT, "80" },
        { STRESS_OP_DELETE, "25" }, { STRESS_OP_DELETE, "60" },
        { STRESS_OP_INSERT, "20" }, { STRESS_OP_INSERT, "70" },
        { STRESS_OP_DELETE, "50" },
        { STRESS_OP_INSERT, "15" }, { STRESS_OP_INSERT, "65" },
        { STRESS_OP_DELETE, "10" }, { STRESS_OP_DELETE, "80" },
        { STRESS_OP_INSERT, "05" }, { STRESS_OP_INSERT, "90" },
        { STRESS_OP_DELETE, "30" }, { STRESS_OP_DELETE, "75" },
    };
    size_t num_ops = sizeof ops / sizeof ops[0];
    size_t expected_size = 0;

    /* invariant: ops[0..i) have already been applied and checked consistent
     * (rb_size matches expected_size, rb_validate holds) after each one */
    for (size_t i = 0; i < num_ops; i++) {
        if (ops[i].type == STRESS_OP_INSERT) {
            CHECK(rb_insert(t, ops[i].key, (void *)ops[i].key) == 0);
            expected_size++;
        } else {
            CHECK(rb_delete(t, ops[i].key) == 0);
            expected_size--;
            CHECK(rb_find(t, ops[i].key) == NULL);
        }
        CHECK(rb_size(t) == expected_size);
        CHECK(rb_validate(t) == 0);
    }

    const char *expect_present[] = { "20", "70", "15", "65", "05", "90" };
    size_t np = sizeof expect_present / sizeof expect_present[0];
    /* invariant: expect_present[0..i) have already been confirmed present with their key pointer as value */
    for (size_t i = 0; i < np; i++) {
        CHECK(rb_find(t, expect_present[i]) == (void *)expect_present[i]);
    }

    const char *expect_absent[] = { "50", "25", "75", "10", "30", "60", "80" };
    size_t na = sizeof expect_absent / sizeof expect_absent[0];
    /* invariant: expect_absent[0..i) have already been confirmed absent */
    for (size_t i = 0; i < na; i++) {
        CHECK(rb_find(t, expect_absent[i]) == NULL);
    }

    CHECK(rb_size(t) == np);

    rb_destroy(t);
}

/* Allocation-failure paths (rb_insert returning -1) aren't practically
 * testable without fault-injecting malloc, so they're intentionally not
 * covered here. */

typedef void (*test_fn)(void);

typedef struct {
    const char *name;
    test_fn     fn;
} test_case_t;

static const test_case_t tests[] = {
    { "create_destroy_empty",                   test_create_destroy_empty },
    { "insert_find_basic",                       test_insert_find_basic },
    { "insert_overwrite_frees_old_and_keeps_size", test_insert_overwrite_frees_old_and_keeps_size },
    { "insert_borrowed_values_not_owned",         test_insert_borrowed_values_not_owned },
    { "validate_bst_ordering",                    test_validate_bst_ordering },
    { "size_tracks_inserts",                      test_size_tracks_inserts },
    { "insert_fixup_line_ascending",              test_insert_fixup_line_ascending },
    { "insert_fixup_line_descending",             test_insert_fixup_line_descending },
    { "insert_fixup_zigzag_left_right",           test_insert_fixup_zigzag_left_right },
    { "insert_fixup_zigzag_right_left",           test_insert_fixup_zigzag_right_left },
    { "insert_fixup_recolor_case",                test_insert_fixup_recolor_case },
    { "insert_fixup_cascading_recolor",           test_insert_fixup_cascading_recolor },
    { "insert_no_stale_root_after_left_rotation", test_insert_no_stale_root_after_left_rotation },
    { "insert_no_stale_root_after_right_rotation", test_insert_no_stale_root_after_right_rotation },
    { "foreach_visits_in_sorted_order",           test_foreach_visits_in_sorted_order },
    { "foreach_values_match",                     test_foreach_values_match },
    { "foreach_empty_tree_calls_nothing",         test_foreach_empty_tree_calls_nothing },
    { "delete_missing_key_empty_tree",            test_delete_missing_key_empty_tree },
    { "delete_missing_key_nonempty_tree",         test_delete_missing_key_nonempty_tree },
    { "delete_only_root",                         test_delete_only_root },
    { "delete_leaf_red",                          test_delete_leaf_red },
    { "delete_node_with_only_left_child",         test_delete_node_with_only_left_child },
    { "delete_node_with_only_right_child",        test_delete_node_with_only_right_child },
    { "delete_node_with_two_children",            test_delete_node_with_two_children },
    { "delete_black_leaf_with_red_sibling_on_left",  test_delete_black_leaf_with_red_sibling_on_left },
    { "delete_black_leaf_with_red_sibling_on_right", test_delete_black_leaf_with_red_sibling_on_right },
    { "delete_frees_owned_value",                 test_delete_frees_owned_value },
    { "delete_does_not_free_borrowed_value",      test_delete_does_not_free_borrowed_value },
    { "delete_then_reinsert_same_key",            test_delete_then_reinsert_same_key },
    { "interleaved_insert_delete_stress",         test_interleaved_insert_delete_stress },
};

static const size_t num_tests = sizeof tests / sizeof tests[0];

static void run_test(const test_case_t *tc) {
    int before = failures;
    printf("RUN  %s\n", tc->name);
    tc->fn();
    if (failures == before) {
        printf("PASS %s\n", tc->name);
    } else {
        printf("FAIL %s\n", tc->name);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        const test_case_t *match = NULL;
        /* invariant: tests[0..i) have already been checked for a name match */
        for (size_t i = 0; i < num_tests; i++) {
            if (strcmp(tests[i].name, argv[1]) == 0) {
                match = &tests[i];
                break;
            }
        }
        if (match == NULL) {
            fprintf(stderr, "no such test: %s\navailable tests:\n", argv[1]);
            /* invariant: tests[0..i) have already had their name printed */
            for (size_t i = 0; i < num_tests; i++) {
                fprintf(stderr, "  %s\n", tests[i].name);
            }
            return 1;
        }
        run_test(match);
    } else {
        /* invariant: tests[0..i) have already been run */
        for (size_t i = 0; i < num_tests; i++) {
            run_test(&tests[i]);
        }
    }

    if (failures > 0) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all checks passed\n");
    return 0;
}
