#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    char buf[4096];
    int want = 100;
    int fd;
    ssize_t n;

    if (argc > 1)
        want = atoi(argv[1]);
    if (want > (int)sizeof(buf) - 1)
        want = sizeof(buf) - 1;

    fd = open("/dev/myDev", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("test_read: asking for %d bytes\n", want);
    n = read(fd, buf, want);
    if (n < 0) {
        perror("read");
    } else if (n == 0) {
        printf("test_read: buffer is empty (read returned 0)\n");
    } else {
        buf[n] = '\0';
        printf("test_read: read %zd bytes: \"%s\"\n", n, buf);
    }

    close(fd);
    return 0;
}
