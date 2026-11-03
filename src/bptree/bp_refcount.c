#include "bp_refcount.h"

#include <stdlib.h>

void* _bp_ref_ref(BpRcBase* self) {
    self->refcount++;
    return self;
}

bool _bp_ref_unref(BpRcBase* self, BpRefDtorCallback dtor) {
    self->refcount--;
    if (self->refcount == 0) {
        if (dtor != NULL) {
            dtor(self);
        } else {
            free(self);
        }

        return false;
    }

    return true;
}
