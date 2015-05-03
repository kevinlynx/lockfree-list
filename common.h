
#ifndef __l_list_common_h
#define __l_list_common_h

#define CAS_V(addr, old, x) __sync_val_compare_and_swap(addr, old, x)
#define CAS(addr, old, x) (CAS_V(addr, old, x) == old)
#define ATOMIC_INC(addr) __sync_fetch_and_add(addr, 1)
#define ATOMIC_ADD(addr, n) __sync_add_and_fetch(addr, n)

#define TRUE 1
#define FALSE 0

#define trace printf

#endif 
