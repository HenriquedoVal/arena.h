#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>


#define ARENA_IMPLEMENTATION
#define ARENA_PAGE_MUL 64  // so vmmap changes are more visible
#include "arena.h"

#define PAGE 4096
#define FULL_BUFFER PAGE * ARENA_PAGE_MUL


static void step(char *msg) {
    printf("%s ", msg);
    getchar();
}


/*
 * I use this thing with vmmap to keep track of mem
 * (bc a LeakSanitizer for MS Windows is an unresolved
 * CS problem in 2024)
*/
static void test_leak(void) {
    Arena a = {0};

    for (int i = 0; i < 10; i++) {
        step("Will increase");
        arena_alloc(&a, FULL_BUFFER);
    }

    arena_reset(&a);

    for (int i = 0; i < 10; i++) {
        step("Should not increase");
        arena_alloc(&a, FULL_BUFFER);
    }

    step("will free");
    arena_free(&a);

    for (int i = 0; i < 3; i++) {
        step("Will increase");
        arena_alloc(&a, FULL_BUFFER);
    }

    arena_reset(&a);
    arena_alloc(&a, FULL_BUFFER);
    step("Should increase by the same amount");
    // the current buffer should be freed and a new one with
    // this size allocated
    arena_alloc(&a, FULL_BUFFER * 2);

    step("Should NOT increase");
    arena_reset(&a);
    arena_alloc(&a, FULL_BUFFER);
    arena_alloc(&a, FULL_BUFFER * 2);
    arena_alloc(&a, FULL_BUFFER);

    step("Will free");
    arena_free(&a);

    step("End");
}


static void test_flow(void) {
    printf("test_flow()\n");

    Arena a = {0};

    char *first = arena_alloc(&a, FULL_BUFFER);
    char *second = arena_alloc(&a, FULL_BUFFER);

    arena_reset(&a);

    char *third = arena_alloc(&a, FULL_BUFFER);
    char *fourth = arena_alloc(&a, FULL_BUFFER);

    assert(first == third);
    assert(second == fourth);

    arena_free(&a);
}


static void test_mark(void) {
    printf("test_mark()\n");

    Arena a = {0};

    char *init = arena_alloc(&a, FULL_BUFFER);

    ArenaMark m1 = arena_mark(&a);

        char *first = arena_alloc(&a, FULL_BUFFER);
        char *second = arena_alloc(&a, 15);

    ArenaMark m2 = arena_mark(&a);

        char *third = arena_alloc(&a, 400);
        char *any_other = arena_alloc(&a, FULL_BUFFER);

    arena_mark_reset(&a, m1);

        char *fourth = arena_alloc(&a, FULL_BUFFER);
        char *fifth = arena_alloc(&a, 15);
        assert(first == fourth);
        assert(second == fifth);

    arena_mark_reset(&a, m2);

        char *seventh = arena_alloc(&a, 100);
        assert(third == seventh);

    arena_free(&a);
}


static void test_strdup(void) {
    printf("test_strdup()\n");

    Arena a = {0};
    const char *test = "abcdefghijklm";
    size_t len = strlen(test);

    char *unused = arena_alloc(&a, FULL_BUFFER - (len / 2));  // so str doesn't fit
    char *one = arena_strdup(&a, test);

    bool ret = memcmp(one, test, strlen(test));
    assert(ret == 0);

    arena_free(&a);
}


static void test_info(void) {
    printf("test_info()\n");

    Arena a = {0};

    arena_alloc(&a, FULL_BUFFER);
    assert(a.info.total_alloc == FULL_BUFFER);

    arena_alloc(&a, 10);
    assert(a.info.total_alloc == FULL_BUFFER * 2);

    arena_free(&a);
    assert(a.info.total_freed == FULL_BUFFER * 2);
}


int main(void) {
    test_flow();
    test_mark();
    test_strdup();
    test_info();

    // test_leak();

    printf("All OK\n");
    return 0;
}
