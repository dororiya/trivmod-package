#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *msg = "hello world";
    int fd;
    ssize_t n;

    if (argc > 1)
        msg = argv[1];

    fd = open("/dev/myDev", O_WRONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("test_write: writing \"%s\" (%zu bytes)\n", msg, strlen(msg));
    n = write(fd, msg, strlen(msg));
    if (n < 0)
        perror("write");
    else
        printf("test_write: wrote %zd bytes\n", n);

    close(fd);
    return 0;
}
