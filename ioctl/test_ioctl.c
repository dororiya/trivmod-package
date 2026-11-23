#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "trivmod_ioctl.h"

int main(int argc, char *argv[])
{
    struct trivmod_stats stats;
    int fd;

    if (argc < 2 || (strcmp(argv[1], "get") != 0 && strcmp(argv[1], "clear") != 0)) {
        printf("usage: %s get | clear\n", argv[0]);
        return 1;
    }

    fd = open("/dev/myDev", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (strcmp(argv[1], "get") == 0) {
        if (ioctl(fd, TRIVMOD_GET_STATS, &stats) < 0) {
            perror("ioctl GET_STATS");
            close(fd);
            return 1;
        }
        printf("statistics:\n");
        printf("  opens         = %d\n", stats.open_count);
        printf("  closes        = %d\n", stats.release_count);
        printf("  users now     = %d\n", stats.users);
        printf("  bytes written = %d\n", stats.bytes_written);
        printf("  bytes read    = %d\n", stats.bytes_read);
        printf("  buffer used   = %d\n", stats.buf_used);
    } else {
        if (ioctl(fd, TRIVMOD_CLEAR_STATS) < 0) {
            perror("ioctl CLEAR_STATS");
            close(fd);
            return 1;
        }
        printf("statistics cleared\n");
    }

    close(fd);
    return 0;
}
