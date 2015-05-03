/*
 * a simple hazard pointer implementation 
 * Kevin Lynx 2015.05.02
 */
#include "haz_ptr.h"
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define HAZ_MAX_COUNT HAZ_MAX_THREAD * HAZ_MAX_COUNT_PER_THREAD

typedef struct free_t {
    void *p;
    struct free_t *next;
} free_t;

static int _tidSeed = 0;
static hazard_t _haz_array[HAZ_MAX_COUNT];
/* a simple link list to save pending free pointers */
static free_t _free_list[HAZ_MAX_THREAD];

static int get_thread_id() {
    static __thread int _tid = -1;
    if (_tid >= 0) return _tid;
    _tid = ATOMIC_INC(&_tidSeed);
    _free_list[_tid].next = NULL;
    return _tid; 
}

static int haz_confict(int self, void *p) {
    int self_p = self * HAZ_MAX_COUNT_PER_THREAD;
    for (int i = 0; i < HAZ_MAX_COUNT; ++i) {
        if (i >= self_p && i < self_p + HAZ_MAX_COUNT_PER_THREAD)
            continue; /* skip self */
        if (_haz_array[i] == p)
            return TRUE;
    }
    return FALSE;
}

hazard_t *haz_get(int idx) {
    int tid = get_thread_id();
    return &_haz_array[tid * HAZ_MAX_COUNT_PER_THREAD + idx];
}

void haz_defer_free(void *p) {
    int tid = get_thread_id();
    free_t *f = (free_t*) malloc(sizeof(*f));
    f->p = p;
    f->next = _free_list[tid].next;
    _free_list[tid].next = f;
    haz_gc();
}

void haz_gc() {
    int tid = get_thread_id();
    free_t *head = &_free_list[tid];
    free_t *pred = head, *next = head->next;
    while (next) {
        if (!haz_confict(tid, next->p)) { /* safe to free */
            free_t *tmp = next->next;
            trace("hazard (%d) free ptr %p\n", tid, next->p);
            free(next->p);
            pred->next = tmp;
            free(next);
            next = tmp;
        } else {
            pred = next;
            next = next->next;             
        }
    }
    if (head->next == NULL) {
        trace("thread %d freed all ptrs\n", tid);
    }
}
