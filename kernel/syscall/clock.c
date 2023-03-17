#include <kernel/api/err.h>
#include <kernel/api/errno.h>
#include <kernel/api/time.h>
#include <kernel/scheduler.h>
#include <kernel/system.h>

int sys_clock_gettime(clockid_t clk_id, struct timespec* tp) {
    switch (clk_id) {
    case CLOCK_REALTIME:
    case CLOCK_MONOTONIC:
        time_now(tp);
        return 0;
    default:
        return -EINVAL;
    }
}

static void timespec_add(struct timespec* this, const struct timespec* other) {
    this->tv_sec += other->tv_sec;
    this->tv_nsec += other->tv_nsec;
    if (this->tv_nsec >= 1000000000) {
        ++this->tv_sec;
        this->tv_nsec -= 1000000000;
    }
}

static void timespec_saturating_sub(struct timespec* this,
                                    const struct timespec* other) {
    this->tv_sec -= other->tv_sec;
    this->tv_nsec -= other->tv_nsec;
    if (this->tv_nsec < 0) {
        --this->tv_sec;
        this->tv_nsec += 1000000000;
    }
    if (this->tv_sec < 0)
        this->tv_sec = this->tv_nsec = 0;
}

static bool sleep_should_unblock(const struct timespec* deadline) {
    struct timespec now = {0};
    time_now(&now);
    return now.tv_sec > deadline->tv_sec ||
           (now.tv_sec == deadline->tv_sec && now.tv_nsec >= deadline->tv_nsec);
}

int sys_clock_nanosleep(clockid_t clockid, int flags,
                        const struct timespec* request,
                        struct timespec* remain) {
    switch (clockid) {
    case CLOCK_REALTIME:
    case CLOCK_MONOTONIC:
        break;
    default:
        return -EINVAL;
    }

    struct timespec deadline = {0};
    switch (flags) {
    case 0: {
        int rc = time_now(&deadline);
        if (IS_ERR(rc))
            return rc;
        timespec_add(&deadline, request);
        break;
    }
    case TIMER_ABSTIME:
        deadline = *request;
        break;
    default:
        return -EINVAL;
    }

    int rc =
        scheduler_block((should_unblock_fn)sleep_should_unblock, &deadline);
    if (IS_ERR(rc))
        return rc;
    if (remain) {
        *remain = deadline;
        struct timespec now;
        int rc = time_now(&now);
        if (IS_ERR(rc))
            return rc;
        timespec_saturating_sub(remain, &now);
    }
    return 0;
}
