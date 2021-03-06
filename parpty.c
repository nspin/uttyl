#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern char *program_invocation_short_name;

static void usage(int code) {
    fprintf(stderr, "Usage: %s [-ioe] DRIVER_SCRIPT SLAVE [SLAVE_ARGS ...]\n", program_invocation_short_name);
    exit(code);
}

int main(int argc, char **argv) {

    int master, slave, pid;
    char master_s[16];

    int fds = 0;
    char *driver_script, **slave_argv;
    int c, i;

    while ((c = getopt(argc, argv, "hioe")) != -1) {
        switch (c) {
            case 'i':
                fds |= 1;
                break;
            case 'o':
                fds |= 2;
                break;
            case 'e':
                fds |= 4;
                break;
            case 'h':
                usage(0);
            default:
                usage(1);
        }
    }

    if (argc - optind < 2) {
        usage(1);
    }

    driver_script = argv[optind];
    slave_argv = argv + optind;
    for (i = optind; i < argc - 1; i++) {
        argv[i] = argv[i + 1];
    }
    argv[argc - 1] = NULL;

    if (openpty(&master, &slave, NULL, NULL, NULL)) {
        return 1;
    }
 
    if ((pid = fork())) {
        close(slave);
        snprintf(master_s, sizeof(master_s), "%d", master);
        execvp("sh", (char *const[]){"sh", "-c", driver_script, master_s, NULL});
    } else {
        close(master);
        setsid();
        if (ioctl(slave, TIOCSCTTY, (char *)NULL)) {
            return 1;
        }
        for (i = 0; i < 3; i++) {
            if (fds & (1 << i)) {
                dup2(slave, i);
            }
        }
        execvp(slave_argv[0], slave_argv);
    }

    return 1;
}
