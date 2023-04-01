#include "api/sys/types.h"
#include "lock.h"
#include <common/extra.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct ring_buf {
    mutex lock;
    void* inner_buf;
    atomic_size_t write_idx;
    atomic_size_t read_idx;
} ring_buf;

NODISCARD int ring_buf_init(ring_buf*);
void ring_buf_destroy(ring_buf*);
bool ring_buf_is_empty(const ring_buf*);
bool ring_buf_is_full(const ring_buf*);
NODISCARD ssize_t ring_buf_write(ring_buf*, const void* bytes, size_t count);
ssize_t ring_buf_write_evicting_oldest(ring_buf*, const void* bytes, size_t count);
NODISCARD ssize_t ring_buf_read(ring_buf*, void* bytes, size_t count);
