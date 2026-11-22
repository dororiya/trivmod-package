#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd1, fd2;

    printf("test: opening /dev/trivdev (1st time)\n");
    fd1 = open("/dev/trivdev", O_RDWR);
    if (fd1 < 0) {
        perror("open 1");
        return 1;
    }
    printf("test: opened, fd1 = %d\n", fd1);

    printf("test: opening /dev/trivdev (2nd time)\n");
    fd2 = open("/dev/trivdev", O_RDWR);
    if (fd2 < 0) {
        perror("open 2");
        close(fd1);
        return 1;
    }
    printf("test: opened, fd2 = %d\n", fd2);

    printf("test: closing fd2\n");
    close(fd2);

    printf("test: closing fd1\n");
    close(fd1);

    printf("test: done\n");
    return 0;
}
