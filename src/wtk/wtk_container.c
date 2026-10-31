#include "wtk_container.h"
#include "wtk_mem.h"

void* wtkvec_try_grow1(void* p, int element_size)
{
    WtkVecHdr* h = wtkvec_hdr(p);
    if ((h->count + 1) > h->capacity) {
        unsigned int new_capacity = (h->capacity * 3) >> 1;

        size_t alloc_size = sizeof(WtkVecHdr) + element_size * new_capacity;
        void* new_h = wtk_realloc(void*, h, alloc_size);

        p = (char*)new_h + sizeof(WtkVecHdr);
        wtkvec_hdr(p)->capacity = new_capacity;
    }

    return p;
}

int wtk_idpool_get(WtkIdpool* p)
{
    if (p == NULL) {
        // todo: we don't handle -1 right now. we probably should?
        return -1;
    }

    if (wtkvec_elem_count(p->free_ids) > 0) {
        int free_id = wtkvec_end(p->free_ids)[-1];
        wtkvec_pop_prim(p->free_ids);
        return free_id;
    } else {
        int free_id = p->current_id;
        p->current_id++;
        return free_id;
    }
}

// does not check if id already exists
void wtk_idpool_relinquish(WtkIdpool* p, int id)
{
    if (p == NULL) {
        return;
    }

    if (id == p->current_id - 1) {
        p->current_id--;
    } else {
        wtkvec_push(p->free_ids, id);
    }
}