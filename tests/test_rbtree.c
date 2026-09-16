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
