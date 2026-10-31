#ifndef WTK_CONTAINER_H
#define WTK_CONTAINER_H

#include <stdlib.h>
#include "wtk_mem.h"
#include "wtk_error.h"

// wtk vector //////////////////////////////////////////////////////////////////
// todo: wtkvec -> wtk_vec?
#define WTKVEC_MIN_CAPACITY 3

#define WtkVec(elem) elem*

typedef void (*wtkvec_elem_destructor)(void* elem);
typedef struct
{
    wtkvec_elem_destructor destructor;
    unsigned int count;
    unsigned int capacity;
} WtkVecHdr;

#define wtkvec_hdr(p) ((WtkVecHdr*)(p) - 1)
#define wtkvec_destructor(p) (wtkvec_hdr(p)->destructor)
#define wtkvec_elem_size(p) sizeof(*(p))
#define wtkvec_elem_count(p) ((p) != NULL ? (wtkvec_hdr((p)))->count : 0)

void* wtkvec_try_grow1(void* p, int element_size);

// workarounds required since we don't know the type of p
#ifdef __cplusplus

#define wtkvec_new(p, cap, destruct)                                                               \
    do {                                                                                           \
        if ((p) == NULL) {                                                                         \
            size_t __wtkvec_alloc_size = ((cap) >= WTKVEC_MIN_CAPACITY)                            \
                ? sizeof(WtkVecHdr) + wtkvec_elem_size(p) * cap                                    \
                : sizeof(WtkVecHdr) + wtkvec_elem_size(p) * WTKVEC_MIN_CAPACITY;                   \
            WtkVecHdr* __wtkvec_new_h = wtk_malloc(WtkVecHdr*, __wtkvec_alloc_size);               \
            __wtkvec_new_h->destructor = destruct;                                                 \
            __wtkvec_new_h->count = 0;                                                             \
            __wtkvec_new_h->capacity = ((cap) >= WTKVEC_MIN_CAPACITY) ? cap : WTKVEC_MIN_CAPACITY; \
            void* __wtkvec_new_p = (char*)__wtkvec_new_h + sizeof(WtkVecHdr);                      \
            memcpy(&(p), &__wtkvec_new_p, sizeof(p));                                              \
        }                                                                                          \
    } while (0)

#define wtkvec_push(p, elem)                                              \
    do {                                                                  \
        if ((p) != NULL) {                                                \
            void* __wtkvec_grown_p = wtkvec_try_grow1((p), sizeof(*(p))); \
            memcpy(&(p), &__wtkvec_grown_p, sizeof(p));                   \
            (p)[wtkvec_hdr(p)->count++] = (elem);                         \
        }                                                                 \
    } while (0)

#else

#define wtkvec_new(p, cap, destruct)                                                               \
    do {                                                                                           \
        if ((p) == NULL) {                                                                         \
            size_t __wtkvec_alloc_size = ((cap) >= WTKVEC_MIN_CAPACITY)                            \
                ? sizeof(WtkVecHdr) + wtkvec_elem_size(p) * cap                                    \
                : sizeof(WtkVecHdr) + wtkvec_elem_size(p) * WTKVEC_MIN_CAPACITY;                   \
            WtkVecHdr* __wtkvec_new_h = wtk_malloc(WtkVecHdr*, __wtkvec_alloc_size);               \
            __wtkvec_new_h->destructor = destruct;                                                 \
            __wtkvec_new_h->count = 0;                                                             \
            __wtkvec_new_h->capacity = ((cap) >= WTKVEC_MIN_CAPACITY) ? cap : WTKVEC_MIN_CAPACITY; \
            (p) = (void*)((char*)__wtkvec_new_h + sizeof(WtkVecHdr));                              \
        }                                                                                          \
    } while (0)

#define wtkvec_push(p, elem)                           \
    do {                                               \
        if ((p) != NULL) {                             \
            (p) = wtkvec_try_grow1((p), sizeof(*(p))); \
            (p)[wtkvec_hdr(p)->count++] = (elem);      \
        }                                              \
    } while (0)

#endif // !__cplusplus

#define wtkvec_pop(p)                                                       \
    do {                                                                    \
        if ((p) != NULL && wtkvec_elem_count(p) > 0) {                      \
            WtkVecHdr* __wtkvec_destructor = wtkvec_destructor(p);          \
            if (__wtkvec_destructor != NULL) {                              \
                __wtkvec_destructor((void*)&(p)[wtkvec_elem_count(p) - 1]); \
            }                                                               \
            wtkvec_hdr(p)->count--;                                         \
        }                                                                   \
    } while (0)

#define wtkvec_pop_prim(p)                             \
    do {                                               \
        if ((p) != NULL && wtkvec_elem_count(p) > 0) { \
            wtkvec_hdr(p)->count--;                    \
        }                                              \
    } while (0)

#define wtkvec_erase(p, idx)                                                             \
    do {                                                                                 \
        if ((p) != NULL && idx < wtkvec_elem_count(p)) {                                 \
            WtkVecHdr* __wtkvec_destructor = wtkvec_destructor(p);                       \
            if (__wtkvec_destructor != NULL) {                                           \
                __wtkvec_destructor((void*)&(p)[idx]);                                   \
            }                                                                            \
            unsigned int __wtkvec_new_count = wtkvec_hdr(p)->count - 1;                  \
            memmove((p) + idx, (p) + idx + 1, wtkvec_elem_size(p) * __wtkvec_new_count); \
            wtkvec_hdr(p)->count = __wtkvec_new_count;                                   \
        }                                                                                \
    } while (0)

#define wtkvec_clear(p)                                                                     \
    do {                                                                                    \
        if ((p) != NULL) {                                                                  \
            WtkVecHdr* __wtkvec_destructor = wtkvec_destructor(p);                          \
            if (__wtkvec_destructor != NULL) {                                              \
                for (int __wtkvec_i = 0; __wtkvec_i < wtkvec_elem_count(p); __wtkvec_i++) { \
                    __wtkvec_destructor((void*)&(p)[i]);                                    \
                }                                                                           \
            }                                                                               \
            wtkvec_hdr(p)->count = 0;                                                       \
        }                                                                                   \
    } while (0)

#define wtkvec_clear_prim(p)          \
    do {                              \
        if ((p) != _Null_) {          \
            wtkvec_hdr(p)->count = 0; \
        }                             \
    } while (0)

// + increases by pointer size, not by byte!
#define wtkvec_begin(p) (p)
#define wtkvec_end(p) (p + wtkvec_elem_count(p))

#define wtkvec_each(p, T, i) T* i = wtkvec_begin(p); i != wtkvec_end(p); i++
// /////////////////////////////////////////////////////////// end wtk vector //

// wtk id pool /////////////////////////////////////////////////////////////////
typedef struct
{
    int* free_ids;
    int current_id;
} WtkIdpool;

#define wtk_idpool_new(p, first_id)         \
    do {                                    \
        (p)->free_ids = NULL;               \
        (p)->current_id = first_id;         \
        wtkvec_new((p)->free_ids, 0, NULL); \
    } while(0)

int wtk_idpool_get(WtkIdpool* p);
void wtk_idpool_relinquish(WtkIdpool* p, int id);
// ////////////////////////////////////////////////////////// end wtk id pool //

#endif // WTK_CONTAINER_H