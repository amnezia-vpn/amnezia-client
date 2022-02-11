#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <err.h>

// prlimit(1) is util-linux 2.21+ and I'm stuck with 2.20 at the moment

int main(int argc, char *argv[])
{
    if (argc != 3)
        errx(EXIT_FAILURE, "Usage: %s <pid> <new-soft-nofile-limit>", argv[0]);

    pid_t pid = atoi(argv[1]);
    rlim_t soft = atoi(argv[2]);

    struct rlimit rl;
    if (prlimit(pid, RLIMIT_NOFILE, NULL, &rl) == -1)
        err(EXIT_FAILURE, "prlimit(%d, RLIMIT_NOFILE, NULL, %p)", pid, &rl);

    if (rl.rlim_max < soft)
        errx(EXIT_FAILURE, "rlim_max = %ld, requested limit = %ld", rl.rlim_max, soft);

    rl.rlim_cur = soft;

    if (prlimit(pid, RLIMIT_NOFILE, &rl, NULL) == -1)
        err(EXIT_FAILURE, "prlimit(%d, RLIMIT_NOFILE, %p, NULL)", pid, &rl);

    return 0;
}
