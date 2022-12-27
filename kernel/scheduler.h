#pragma once

#include "api/sys/types.h"
#include "forward.h"
#include <common/extra.h>
#include <stdbool.h>

void scheduler_init(void);

void scheduler_yield(bool requeue_current);
void scheduler_register(struct process*);
void scheduler_unregister(struct process*);
void scheduler_enqueue(struct process*);
void scheduler_tick(bool in_kernel);

typedef bool (*should_unblock_fn)(void*);
NODISCARD int scheduler_block(should_unblock_fn should_unblock, void* data);
