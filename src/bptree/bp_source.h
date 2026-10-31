#ifndef BP_SOURCE_H
#define BP_SOURCE_H

#include <stdio.h>

#include "wtk_common.h"
#include "bp_refcount.h"
#include "bp_defines.h"
#include "wtk_file.h"

typedef enum {
    BP_SRC_FILE,
    BP_SRC_BUFFER,
} BpSourceKind;

typedef struct {
    WtkFile* handle;
    int64_t cached_length;
} BpFile;

typedef struct {
    uint8_t* data;
    bp_len_t length;
    bp_len_t capacity;
} BpBuffer;

typedef struct {
    BpRcBase rc;
    BpSourceKind kind;
    union {
        BpFile file;
        BpBuffer buf;
    };
} BpSource;

bp_len_t bp_buffer_append(BpBuffer* self, const uint8_t* data, bp_len_t length);

BpSource* bp_source_new_as_file(WtkFile* file);
BpSource* bp_source_new_as_buffer(bp_len_t capacity);

#define bp_source_ref(self) bp_ref_ref((self), BpSource)
void bp_source_unref(BpSource* self);

bp_len_t bp_source_length(const BpSource* self);
bp_len_t bp_source_capacity(const BpSource* self);

bp_len_t bp_source_append(BpSource* self, const uint8_t* data, bp_len_t size);
bp_len_t bp_source_read(const BpSource* self, uint8_t* out_data, bp_len_t offset, bp_len_t length);

#endif // BP_SOURCE_H
