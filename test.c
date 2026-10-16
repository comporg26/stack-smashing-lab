#include <stdio.h>
#include <stdint.h>

__attribute__((noinline))
void function(int a, int b, int c)
{
    uintptr_t *ret;

    /* start at the frame pointer (rbp) */
    ret = (uintptr_t *)__builtin_frame_address(0);

    /* task 1: change 0 so ret points to the saved return address */
    ret = ret + 0;

    /* task 2: change 0 to the number of instruction bytes to skip */
    *ret = *ret + 0;
}

int main(void)
{
    int x;

    x = 0;
    function(1, 2, 3);
    x = 1;
    printf("%d\n", x);

    return 0;
}
