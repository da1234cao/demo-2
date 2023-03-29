#include "list.h"
#include <stdio.h>

/**
 * 链表相乘：(1+2x-4x^4) * (2-2x+5x^4) = (2+2x-4x^2-3x^4+18x^5-20x^8)
 * 输入时依次输入每项系数和指数(系数和指数都为0结束)：
 * 注：输入假定，一元多项式已按照指数进行非降序排列
 * 1 0 2 1 -4 4
 * 2 0 -2 1 5 4
 * 输出：
 * 2 0 2 1 -4 2 -3 4 18 5 -20 8
 */

typedef struct _node {
  struct list_head list;
  int coeff; // 系数
  int expon; // 指数
} node;

void print_linked(node *head) {
  struct list_head *pos;
  list_for_each(pos, &head->list) {
    node *entry = list_entry(pos, node, list);
    printf("coeff: %d, expon: %d\n", entry->coeff, entry->expon);
  }
}

void create_linked(node *head, int info[], int len) {
  for (int i = 0; i < len; i += 2) {
    if ((i + 1 < len) && (info[i] != 0 || info[i + 1] != 0)) {
      node *elem = (node *)malloc(sizeof(node));
      elem->coeff = info[i];
      elem->expon = info[i + 1];
      list_add_tail(&elem->list, &head->list);
    } else {
      ;
    }
  }
}

void destroy_linked(node *head) {
  node *elem, *tmp;
  list_for_each_entry_safe(elem, tmp, &head->list, list) {
    list_del(&elem->list);
    free(elem);
  }
}

void multi_linked(node *linked_a, node *linked_b, node *linked_c) {
  struct list_head *pos_a;
  struct list_head *pos_b;
  struct list_head *pos_c;

  list_for_each(pos_a, &linked_a->list) {
    node *entry_a = list_entry(pos_a, node, list);
    int coeff_a = entry_a->coeff;
    int expon_a = entry_a->expon;
    list_for_each(pos_b, &linked_b->list) {
      node *entry_b = list_entry(pos_b, node, list);
      int coeff_b = entry_b->coeff;
      int expon_b = entry_b->expon;

      node *elem = (node *)malloc(sizeof(node));
      elem->coeff = coeff_a * coeff_b;
      elem->expon = expon_a + expon_b;
      if (list_empty(&linked_c->list)) { // 存储结果的链表为空，直接插入
        list_add_tail(&elem->list, &linked_c->list);
      } else {
        list_for_each(pos_c, &linked_c->list) {
          node *entry_c = list_entry(pos_c, node, list);
          int coeff_c = entry_c->coeff;
          int expon_c = entry_c->expon;
          if (elem->expon < expon_c) {
            // 指数非降序，遇到第一个大于elem的指数，插入它之前
            list_add(&elem->list, pos_c->prev);
            break;
          } else if (elem->expon == expon_c) {
            // 指数非降序，遇到等于elem的指数，合并
            entry_c->coeff += coeff_c;
            break;
          } else {
            // 指数非降序，遇到小于elem的指数，通常是继续向后找
            // 如果此时是查找的最后一个元素--即,elem的指数最大，直接插入到最后
            if (pos_c->next == &linked_c->list) {
              list_add(&elem->list, pos_c);
              break;
            }
          }
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  node linked_a;
  node linked_b;
  node linked_c;

  INIT_LIST_HEAD(&linked_a.list);
  INIT_LIST_HEAD(&linked_b.list);
  INIT_LIST_HEAD(&linked_c.list);

  int a_info[] = {1, 0, 2, 1, -4, 4};
  int b_info[] = {2, 0, -2, 1, 5, 4};

  create_linked(&linked_a, a_info, sizeof(a_info) / sizeof(a_info[0]));
  print_linked(&linked_a);
  printf("----------------------\n");
  create_linked(&linked_b, b_info, sizeof(b_info) / sizeof(b_info[0]));
  print_linked(&linked_b);
  printf("----------------------\n");

  multi_linked(&linked_a, &linked_b, &linked_c);
  print_linked(&linked_c);

  destroy_linked(&linked_a);
  destroy_linked(&linked_b);
  destroy_linked(&linked_c);

  return 0;
}