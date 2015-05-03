/*
 * lock-free order-list sample implementation based on 
 * http://www.research.ibm.com/people/m/michael/spaa-2002.pdf
 * Kevin Lynx 2015.05.02
 */

#ifndef __l_list_h
#define __l_list_h

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif 

/* test purpose */
typedef int key_t;
typedef int value_t;
#define NULL_VALUE 0

#define KEY_CMP(k1, k2) (k1 - k2)

typedef struct node_t {
    key_t key;
    value_t val;
    struct node_t *next;
} node_t;

node_t *l_alloc();
void l_free(node_t *head);
int l_insert(node_t *head, key_t key, value_t val);
int l_remove(node_t *head, key_t key);
int l_count(node_t *head);
int l_exist(node_t *head, key_t key);
value_t l_lookup(node_t *head, key_t key);

#ifdef __cplusplus
}
#endif 

#endif
