#ifndef BP_REFCOUNT_H
#define BP_REFCOUNT_H

#include "wtk_common.h"

typedef struct {
    int refcount;
} BpRcBase;

typedef void (*BpRefDtorCallback)(void* obj);

void* _bp_ref_ref(BpRcBase* self);
bool _bp_ref_unref(BpRcBase* self, BpRefDtorCallback dtor);

#define bp_ref_init(obj) (obj)->rc.refcount = 1;

#define bp_ref_ref(obj, T) (T*)_bp_ref_ref(&(obj)->rc)
#define bp_ref_unref(obj, dtor) _bp_ref_unref(&(obj)->rc, dtor)
// todo: support dtor callback... nobody supports this right now ^

#endif // BP_REFCOUNT_H
