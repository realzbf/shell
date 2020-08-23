#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cat_fd(int fd) {
    char buf[4096];
    ssize_t nread, ntotalwritten, nwritten;

    while ((nread = read(fd, buf, sizeof(buf))) > 0) {
        ntotalwritten = 0;
        while (ntotalwritten < nread) {
            nwritten = write(STDOUT_FILENO, buf + ntotalwritten, nread - ntotalwritten);
            if (nwritten < 1)
                fprintf(stderr, "cat: %d: %s\n", fd, strerror(errno));
            ntotalwritten += nwritten;
        }
    }

    return nread == 0 ? 0 : -1;
}

int cat(const char *fname) {
    int fd;// 文件描述符
    int success;
    /************************************
    * O_RDONLY：只读模式
    * O_WRONLY：只写模式
    * O_RDWR：可读可写
    *************************************/
    if ((fd = open(fname, O_RDONLY)) == -1){
        fprintf(stderr, "cat: %s: %s\n", fname, strerror(errno));
        return -1;
    }

    success = cat_fd(fd);

    if (close(fd) != 0)
    return -1;

    return success;
}


int main(int argc, char **argv) {
    int i;

    if (argc == 1) {
        /* 标准输入输出 */
        if (cat_fd(STDIN_FILENO) != 0)
            return 1;
    } else {
        /* 读文件并输出文件内容 */
        for (i = 1; i < argc; i++) {
            if (cat(argv[i]) != 0)
                return 1;
        }
    }
    
    return 0;
}
