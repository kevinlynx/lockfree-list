/*
 * lock-free order-list sample implementation based on 
 * http://www.research.ibm.com/people/m/michael/spaa-2002.pdf
 * Kevin Lynx 2015.05.02
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "common.h"
#include "l_list.h"
#include "haz_ptr.h"

typedef size_t markable_t;

#define MASK ((sizeof(node_t) - 1) & ~0x01)

#define GET_TAG(p) ((markable_t)p & MASK)
#define TAG(p, tag) ((markable_t)p | (tag))
#define MARK(p) ((markable_t)p | 0x01)
#define HAS_MARK(p) ((markable_t)p & 0x01)
#define STRIP_MARK(p) ((node_t*)((markable_t)p & ~(MASK | 0x01)))

int l_find(node_t **pred_ptr, node_t **item_ptr, node_t *head, key_t key);

node_t *l_alloc() {
    node_t *head = (node_t*) malloc(sizeof(*head));    
    trace("alloc list %p\n", head);
    head->next = NULL;
    return head;
}

void l_free(node_t *head) {
    node_t *item = head->next;     
    while (item) {
        node_t *next = STRIP_MARK(item->next);
        free((STRIP_MARK(item)));
        item = next;
    }
    free(head);
}

value_t l_insert(node_t *head, key_t key, value_t val) {
    node_t *pred, *item, *new_item;
    while (TRUE) {
        if (l_find(&pred, &item, head, key)) { /* update its value */
            /* 更新值时，使用item->val互斥，NULL_VALUE表示被删除 */
            node_t *sitem = STRIP_MARK(item);
            value_t old_val = sitem->val;
            while (old_val != NULL_VALUE) {
                value_t ret = CAS_V(&sitem->val, old_val, val);
                if (ret == old_val) {
                    trace("update item %p value success\n", item);
                    return ret;
                }
                trace("update item lost race %p\n", item);
                old_val = ret;
            }
            trace("item %p removed, retry\n", item);
            continue; /* the item has been removed, we retry */
        }
        new_item = (node_t*) malloc(sizeof(node_t));
        new_item->key = key;
        new_item->val = val;
        new_item->next = item;
        /* a). pred是否有效问题：无效只会发生在使用pred->next时，而l_remove正移除该pred
               就会在CAS(&item->next)中竞争，如果remove中成功，那么insert CAS就失败，
               然后重试；反之，remove失败重试，所以保证了pred有效
           b). item本身增加tag一定程度上降低ABA问题几率
        */
        if (CAS(&pred->next, item, new_item)) {
            trace("insert item %p(next) %p success\n", item, new_item);
            return val;
        }
        trace("cas item %p failed, retry\n", item);
        free(new_item);
    }
    return NULL_VALUE;
}

int l_remove(node_t *head, key_t key) {
    node_t *pred, *item, *sitem;
    while (TRUE) {
        if (!l_find(&pred, &item, head, key)) {
            trace("remove item failed %d\n", key);
            return FALSE;
        }
        sitem = STRIP_MARK(item);
        node_t *inext = sitem->next;
        /* 先标记再删除 */
        if (!CAS(&sitem->next, inext, MARK(inext))) {
            trace("cas item %p mark failed\n", sitem->next);
            continue;
        }
        sitem->val = NULL_VALUE;
        int tag = GET_TAG(pred->next) + 1;
        if (CAS(&pred->next, item, TAG(STRIP_MARK(sitem->next), tag))) {
            trace("remove item %p success\n", item);
            haz_defer_free(sitem);
            return TRUE;
        }
        trace("cas item remove item %p failed\n", item);
    }
    return FALSE;
}

int l_find(node_t **pred_ptr, node_t **item_ptr, node_t *head, key_t key) {
    node_t *pred = head;
    node_t *item = head->next;
    /* pred和next会被使用，所以进行标记 */
    hazard_t *hp1 = haz_get(0);
    hazard_t *hp2 = haz_get(1);
    while (item) {
        node_t *sitem = STRIP_MARK(item);
        haz_set_ptr(hp1, STRIP_MARK(pred));
        haz_set_ptr(hp2, STRIP_MARK(item));
        /* 
         如果已被标记，那么紧接着item可能被移除链表甚至释放，所以需要重头查找
        */
        if (HAS_MARK(sitem->next)) { 
            trace("item->next %p %p marked, retry\n", item, sitem->next);
            return l_find(pred_ptr, item_ptr, head, key);
        }
        int d = KEY_CMP(sitem->key, key);
        if (d >= 0) {
            trace("item %p match key %d, pred %p\n", item, key, pred);
            *pred_ptr = pred;
            *item_ptr = item;
            return d == 0 ? TRUE : FALSE;
        }
        pred = item;
        item = sitem->next;
    } 
    trace("not found key %d\n", key);
    *pred_ptr = pred;
    *item_ptr = NULL;
    return FALSE;
}

int l_exist(node_t *head, key_t key) {
    node_t *pred, *item;
    return l_find(&pred, &item, head, key);
}

int l_count(node_t *head) {
    int cnt = 0;
    node_t *item = STRIP_MARK(head->next);
    while (item) {
        if (!HAS_MARK(item->next)) {
            cnt ++;
        }
        item = STRIP_MARK(item->next);
    }
    return cnt;
}    

value_t l_lookup(node_t *head, key_t key) {
    node_t *pred, *item;
    if (l_find(&pred, &item, head, key)) {
        return STRIP_MARK(item)->val;
    }
    return NULL_VALUE;
}
