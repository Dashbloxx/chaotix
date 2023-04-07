#include "random.h"
#include "api/err.h"
#include "api/sys/sysmacros.h"
#include "fs/fs.h"
#include "memory/memory.h"
#include "system.h"
#include <common/string.h>

/*
 *  The reason why the random character device is created here, is because theoretically there can be actual hardware for generating
 *  random numbers that don't depend on other factors (HRNGs which get randomness by observing radioactive decay). 
 */

static int rand_state = 1;

int rand(void) {
    rand_state = ((rand_state * 1103515245U) + 12345U) & 0x7fffffff;
    return rand_state;
}

void srand(unsigned seed) { rand_state = seed == 0 ? 1 : seed; }

static int write_to_bit_bucket(file_description* desc, const void* buffer, unsigned int count) {
    (void)desc;
    (void)buffer;
    return count;
}

static int read_random(file_description* desc, void* buffer, unsigned int count) {
    (void)desc;
    for (unsigned int i = 0; i < count; i++) {
        ((char*)buffer)[i] = (char)rand();
    }
    return count;
}

struct inode* random_device_create() {
    struct inode* inode = kmalloc(sizeof(struct inode));
    if (!inode)
        return ERR_PTR(-ENOMEM);

    static file_ops fops = {.read = read_random, .write = write_to_bit_bucket};
    *inode = (struct inode){.fops = &fops,
                            .mode = S_IFCHR,
                            .device_id = makedev(1, 9),
                            .ref_count = 1};
    return inode;
}