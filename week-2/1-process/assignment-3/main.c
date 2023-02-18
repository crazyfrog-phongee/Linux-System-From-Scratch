#include <stdio.h>
#include <stdlib.h>

void foo()
{
    int a;  /* Stack frame of foo() function */
    int *ptr = &a;  /* Stack frame of foo() function */

    *ptr = 17;
    printf("*ptr = %d\n", *ptr);
    
    free(ptr);
}

/* Accessing an address that is freed or deleted */
void case2(void)
{
    int* p = malloc(8);
    
    *p = 100;

    free(p);

    *p = 200;
}

/* Accessing out of array index bounds */
int case3()
{
    int arr[2] = {1, 2};
    arr[3] = 3;
    return arr[0];
}

int main(int argc, char const *argv[])
{
    // foo();
    // case3();
    return 0;
}
