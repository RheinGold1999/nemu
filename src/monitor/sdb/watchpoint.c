/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
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

#define NR_WP 32
#define EXPR_LEN 256

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  struct watchpoint *prev;
  char expr[EXPR_LEN];
  word_t val;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *tail = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].prev = (i == 0 ? NULL : &wp_pool[i - 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
  if (free_ == NULL) {
    printf("WP alloction failed: wp_pool is used up.\n");
    return NULL;
  }

  WP *wp = free_;
  free_ = wp->next;

  wp->next = NULL;
  wp->prev = tail;

  if (head == NULL) {
    assert(tail == NULL);
    head = wp;
    tail = wp;
  } else {
    assert(tail != NULL);
    tail->next = wp;
    tail = wp;
  }

  return wp;
}

void free_wp(WP *wp) {
  if (head == wp) {
    assert(wp->prev == NULL);
    head = wp->next;
  } else {
    assert(wp->prev != NULL);
    wp->prev->next = wp->next;
  }

  if (tail == wp) {
    assert(wp->next == NULL);
    tail = wp->prev;
  } else {
    assert(wp->next != NULL);
    wp->next->prev = wp->prev;
  }

  wp->next = free_;
  free_ = wp;
}

void free_wp_by_id(int id) {
  if (0 <= id && id < NR_WP) {
    free_wp(&wp_pool[id]);
  } else {
    printf("Failed, id is out of range: %d\n", id);
  }
}

WP* get_wp_head() {
  return head;
}

WP* get_wp_next(const WP *wp) {
  if (wp != NULL) {
    return wp->next;
  }
  return NULL;
}

word_t get_wp_val(const WP *wp) {
  assert(wp != NULL);
  return wp->val;
}

void set_wp_val(WP *wp, word_t val) {
  assert(wp != NULL);
  wp->val = val;
}

int get_wp_id(const WP *wp) {
  assert(wp != NULL);
  return wp->NO;
}

const char* get_wp_expr(const WP *wp) {
  assert(wp != NULL);
  return wp->expr;
}

void set_wp_expr(WP *wp, const char *e) {
  assert(wp != NULL);
  assert(strlen(e) < EXPR_LEN);
  strcpy(wp->expr, e);
}

void print_wp(const WP *wp) {
  printf("Watchpoint %d: %s\n", get_wp_id(wp), wp->expr);
}

void free_all_wp() {
  if (tail != NULL) {
    tail->next = free_;
    free_->prev = tail;
    free_ = head;
    head = NULL;
    tail = NULL;
  } else {
    assert(head == NULL);
  }
}
