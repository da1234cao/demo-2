#include "list.h"
#include <stdio.h>

/**
 * 链表相乘：(1+2x-4x^4) * (2-2x+5x^4) = (2+2x-4x^2-3x^4+18x^5-20x^8)
 * 输入时依次输入每项系数和指数(系数和指数都为0结束)：
 * 1 0 2 1 -4 4
 * 2 0 -2 1 5 4
 * 输出：
 * 2 0 2 1 -4 2 -3 4 18 5 -20 8 
*/

typedef struct _node{
    struct list_head list;
    int coeff; // 系数
    int expon; // 指数
}node;

void create_linked(node* head, int info[], int len){

}

void destroy_linked(node *head) {};

int main(int argc, char* argv[]) {
    node linked_a;
    node linked_b;
    node linked_c;

    INIT_LIST_HEAD(&linked_a.list);
    INIT_LIST_HEAD(&linked_b.list);
    INIT_LIST_HEAD(&linked_c.list);

    int a_info[] = {1, 0, 2, 1, -4, 4};
    int b_info[] = {2, 0, -2, 1, 5, 4};
    printf("a_info size : %d\n", sizeof(a_info)/sizeof(a_info[0]));
}