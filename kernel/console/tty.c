#include <kernel/api/signum.h>
#include <kernel/process.h>

void tty_maybe_send_signal(pid_t pgid, char ch) {
    int unused;
    (void)unused;
    switch (ch) {
    case 'C' - '@':
        unused = process_send_signal_to_group(pgid, SIGINT);
        break;
    case '\\' - '@':
        unused = process_send_signal_to_group(pgid, SIGQUIT);
        break;
    }
}
