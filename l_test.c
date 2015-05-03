/*
 * lock-free order-list sample implementation based on 
 * http://www.research.ibm.com/people/m/michael/spaa-2002.pdf
 * Kevin Lynx 2015.05.02
 */

#include "l_list.h"
#include "common.h"
#include "haz_ptr.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define ASSERT assert

int _wait = 0;
int _tcount = 10;
int _ttimes = 1000;

void *worker(void *u) {
    node_t *list = (node_t*) u;    
    (void)ATOMIC_ADD(&_wait, -1);
    unsigned int seed = (unsigned int) time(NULL);
    int max = _ttimes;
    do {} while (_wait > 0);
    for (int i = 1; i <= _ttimes; ++i) {
        int r = rand_r(&seed) % max;
        if (r % 2) {
            l_insert(list, r, i);
        } else {
            l_remove(list, r);
        }
    }
    printf("list size: %d\n", l_count(list));
    haz_gc();
    return NULL;
}

int main(int argc, char **argv) {
    {
        node_t *list = l_alloc();
        ASSERT(l_insert(list, 1, 1) == 1);
        ASSERT(l_insert(list, 2, 2) == 2);
        ASSERT(l_insert(list, 2, 3) == 2);
        ASSERT(l_lookup(list, 1) == 1); 
        ASSERT(l_lookup(list, 2) == 3); 
        ASSERT(l_remove(list, 3) == FALSE);
        ASSERT(l_remove(list, 1) == TRUE);
        ASSERT(l_lookup(list, 1) == NULL_VALUE); 
        ASSERT(l_insert(list, 1, 2) == 2);
        l_free(list);
    }
    {
        node_t *list = l_alloc();
        if (argc > 1)
            _tcount = atoi(argv[1]);
        if (argc > 2)
            _ttimes = atoi(argv[2]);
        pthread_t *thread = (pthread_t*) malloc(sizeof(pthread_t*) * _tcount);
        _wait = _tcount;
        printf("thread count %d, loop count %d\n", _tcount, _ttimes);
        for (int i = 0; i < _tcount; ++i) {
            pthread_create(&thread[i], NULL, worker, list);
        }
        for (int i = 0; i < _tcount; ++i) {
            pthread_join(thread[i], NULL);
        }
        l_free(list);
        free(thread); /* */
    }
    return 0;
}
