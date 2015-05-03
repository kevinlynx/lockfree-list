#ifndef __haz_ptr_h
#define __haz_ptr_h

typedef void* hazard_t;

#define HAZ_MAX_THREAD 1024
#define HAZ_MAX_COUNT_PER_THREAD 4

hazard_t *haz_get(int idx);
void haz_defer_free(void *p);
void haz_gc();
#define haz_set_ptr(haz, p) (*(void**)haz) = p

#endif 
