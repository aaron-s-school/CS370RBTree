#include <stdio.h>
#include <stdlib.h>
#include <rbtree.h>
#include <assert.h>
#include <stdbool.h>

#define KEY_SPACE 256

int main(int argc, char *argv[]) {
    int iterations = argc > 1 ? atoi(argv[1]) : 10000;
    srand(300);

    rbtree_t *t = rb_create(free);
    if (t == NULL) {
        fprintf(stderr, "rb_create failed\n");
        return 1;
    }

    /* reference model: present[k] tracks whether "key<k>" is currently in t,
     * so assertions below check real expected outcomes instead of guessing */
    bool present[KEY_SPACE] = { false };

    /* invariant: each iteration exercises insert, find, and delete against independently random keys */
    for (int i = 0; i < iterations; i++) {
        int k_ins = rand() % KEY_SPACE;
        char ins_keybuf[32];
        snprintf(ins_keybuf, sizeof ins_keybuf, "key%d", k_ins);
        int *ins_val = malloc(sizeof(int));
        if (ins_val != NULL) {
            *ins_val = k_ins;
            if (rb_insert(t, ins_keybuf, ins_val) != 0) {
                free(ins_val);
            } else {
                present[k_ins] = true;
            }
        }else{
            printf("ins_val is null");
        }

        int k_find = rand() % KEY_SPACE;
        char find_keybuf[32];
        snprintf(find_keybuf, sizeof find_keybuf, "key%d", k_find);
        int *found = rb_find(t, find_keybuf);
        if (present[k_find]) {
            assert(found != NULL);
            assert(*found == k_find);
        } else {
            assert(found == NULL);
        }

        int k_del = rand() % KEY_SPACE;
        char del_keybuf[32];
        snprintf(del_keybuf, sizeof del_keybuf, "key%d", k_del);
        int del = rb_delete(t,del_keybuf);
        assert(del == (present[k_del] ? 0 : -1));
        present[k_del] = false;

        if(i%100 == 0){
            assert(rb_validate(t) == 0);
        }
    }
    rb_destroy(t);

    return 0;
}
