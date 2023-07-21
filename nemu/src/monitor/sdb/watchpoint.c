/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"
#include "watchpoint.h"

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
  if(free_ == NULL) {
    printf("No more watchpoint\n");
    assert(0);
  }
  WP *p = free_;
  free_ = free_->next;
  p->next = head;
  head = p;
  return p;
}

int free_wp(WP *wp) {
  WP *p = head;
  if(p == wp) {
    head = head->next;
    wp->next = free_;
    free_ = wp;
    return 1;
  }
  while(p->next != NULL) {
    if(p->next == wp) {
      p->next = p->next->next;
      wp->next = free_;
      free_ = wp;
      return 1;
    }
    p = p->next;
  }
  printf("No such watchpoint\n");
  return 0;
}

WP* find_wp(int NO) {
  WP *p = head;
  while(p != NULL) {
    if(p->NO == NO) {
      return p;
    }
    p = p->next;
  }
  printf("No such watchpoint\n");
  return NULL;
}

void watchpoints_display() {
    WP *p = head;
    printf("Num\tWhat\n");
    while(p != NULL) {
      printf("%d\t%s\n", p->NO, p->expr);
      p = p->next;
    }
}
