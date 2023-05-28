#include <stdio.h>

typedef struct test {
    int a;
    int b;
    struct test *next;
} test_t;

int test_func(test_t *arg, test_t *next) {
    arg->a = 1;
    arg->b = 2;
    next->a = 1;
    next->b = 2;
    arg->next = next;
    next->next = arg;
}

int main() {
    test_t t1, t2;
    test_func(&t1, &t2);
    printf("t1.a: %d\n", t1.a);
    printf("t1.b: %d\n", t1.b);
    printf("t1.next->a: %d\n", t1.next->a);
    printf("t1.next->b: %d\n", t1.next->b);
}