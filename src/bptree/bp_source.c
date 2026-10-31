#include <stdlib.h>
#include <string.h>

#include "bp_source.h"
#include "wtk_mem.h"

bp_len_t bp_buffer_append(BpBuffer* self, const uint8_t* data, bp_len_t length)
{
    if (length == 0)
        return self->length;

    bp_len_t new_length = self->length + length;
    wtk_assert(new_length <= self->capacity);

    bp_len_t old_length = self->length;
    memcpy(self->data + old_length, data, (size_t)length);
    self->length = new_length;

    return old_length;
}

BpSource* bp_source_new_as_file(WtkFile* file)
{
    BpSource* source = wtk_malloc(BpSource*, sizeof(BpSource));
    bp_ref_init(source);
    source->kind = BP_SRC_FILE;

    BpFile* bp_file = &source->file;
    bp_file->handle = file;
    bp_file->cached_length = wtk_file_length(file);

    return source;
}

BpSource* bp_source_new_as_buffer(bp_len_t capacity)
{
    BpSource* source = wtk_malloc(BpSource*, sizeof(BpSource));
    bp_ref_init(source);
    source->kind = BP_SRC_BUFFER;

    BpBuffer* bp_buffer = &source->buf;
    bp_buffer->data = wtk_malloc(uint8_t*, (size_t)capacity);
    bp_buffer->length = 0;
    bp_buffer->capacity = capacity;

    return source;
}

void bp_source_destroy(void* object)
{
    BpSource* self = (BpSource*)object;

    if (self->kind == BP_SRC_FILE)
        wtk_file_close(self->file.handle);
    else if (self->kind == BP_SRC_BUFFER)
        free(self->buf.data);

    free(self);
}

void bp_source_unref(BpSource* self)
{
    if (self != NULL) {
        bp_ref_unref(self, bp_source_destroy);
    }
}

bp_len_t bp_source_length(const BpSource* self)
{
    if (self->kind == BP_SRC_FILE) {
        return self->file.cached_length;
    } else if (self->kind == BP_SRC_BUFFER) {
        return self->buf.length;
    }

    return 0;
}

bp_len_t bp_source_capacity(const BpSource* self)
{
    if (self->kind == BP_SRC_FILE) {
        return 0;
    } else if (self->kind == BP_SRC_BUFFER) {
        return self->buf.capacity;
    }

    return 0;
}

bp_len_t bp_source_append(BpSource* self, const uint8_t* data, bp_len_t size)
{
    wtk_assert(self->kind == BP_SRC_BUFFER);
    wtk_assert(self->buf.length + size <= self->buf.capacity);

    return bp_buffer_append(&self->buf, data, size);
}

bp_len_t bp_source_read(const BpSource* self, uint8_t* out_data, bp_len_t offset, bp_len_t length)
{
    if (out_data == NULL || offset < 0 || length < 0)
        return -1;

    if (length == 0)
        return 0;

    if (self->kind == BP_SRC_FILE) {
        if (offset > self->file.cached_length)
            return -1;

        int64_t read_at_res = wtk_file_read_at(self->file.handle, out_data, offset, length);
        if (read_at_res == -1)
            return -1;

        return read_at_res;
    } else if (self->kind == BP_SRC_BUFFER) {
        if (offset > self->buf.length)
            return -1;
        
        size_t copy_len = (offset + length > self->buf.length)
            ? self->buf.length - offset
            : length;

        memcpy(out_data, self->buf.data + offset, copy_len);
        return length;
    }

    return -1;
}
