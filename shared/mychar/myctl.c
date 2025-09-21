#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#define MYCHAR_IOC_MAGIC     'M'
#define MYCHAR_IOC_SET_BUFSZ _IOW(MYCHAR_IOC_MAGIC, 1, unsigned long)
#define MYCHAR_IOC_GET_BUFSZ _IOR(MYCHAR_IOC_MAGIC, 2, unsigned long)
#define MYCHAR_IOC_CLEAR     _IO(MYCHAR_IOC_MAGIC,  3)
#define MYCHAR_IOC_GET_LEN   _IOR(MYCHAR_IOC_MAGIC, 4, unsigned long)

static void die(const char *m){ perror(m); exit(1); }

int main(int argc, char **argv)
{
    int fd = open("/dev/mychar", O_RDWR);
    if (fd < 0) die("open /dev/mychar");

    if (argc == 2 && !strcmp(argv[1], "getsz")) {
        unsigned long v=0;
        if (ioctl(fd, MYCHAR_IOC_GET_BUFSZ, &v) < 0) die("GET_BUFSZ");
        printf("%lu\n", v);
    } else if (argc == 3 && !strcmp(argv[1], "setsz")) {
        unsigned long v = strtoul(argv[2], NULL, 0);
        if (ioctl(fd, MYCHAR_IOC_SET_BUFSZ, &v) < 0) die("SET_BUFSZ");
        puts("OK");
    } else if (argc == 2 && !strcmp(argv[1], "getlen")) {
        unsigned long v=0;
        if (ioctl(fd, MYCHAR_IOC_GET_LEN, &v) < 0) die("GET_LEN");
        printf("%lu\n", v);
    } else if (argc == 2 && !strcmp(argv[1], "clear")) {
        if (ioctl(fd, MYCHAR_IOC_CLEAR) < 0) die("CLEAR");
        puts("OK");
    } else {
        fprintf(stderr,
            "Usage:\n"
            "  %s getsz\n"
            "  %s setsz <bytes>\n"
            "  %s getlen\n"
            "  %s clear\n", argv[0], argv[0], argv[0], argv[0]);
        return 2;
    }

    close(fd);
    return 0;
}

