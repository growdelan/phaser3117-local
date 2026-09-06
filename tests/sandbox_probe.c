/* Exercise the exact production confine() function, without printing. */
#define main driver_main
#include "../src/rasterto3117.c"
#undef main
#include <sys/socket.h>
#include <netinet/in.h>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    confine();
    errno = 0;
    if (!strcmp(argv[1], "read")) {
        int fd = open("/etc/passwd", O_RDONLY);
        return (fd == -1 && (errno == EPERM || errno == EACCES)) ? 0 : 1;
    }
    if (!strcmp(argv[1], "write")) {
        int fd = open("/private/tmp/phaser3117-sandbox-probe", O_WRONLY|O_CREAT, 0600);
        return (fd == -1 && (errno == EPERM || errno == EACCES)) ? 0 : 1;
    }
    if (!strcmp(argv[1], "network")) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return (errno == EPERM || errno == EACCES) ? 0 : 1;
        struct sockaddr_in address = {0};
        address.sin_family = AF_INET; address.sin_port = htons(9);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        int result = connect(fd, (struct sockaddr *)&address, sizeof address);
        return (result == -1 && (errno == EPERM || errno == EACCES)) ? 0 : 1;
    }
    if (!strcmp(argv[1], "exec")) {
        execl("/usr/bin/true", "true", (char *)NULL);
        return (errno == EPERM || errno == EACCES) ? 0 : 1;
    }
    return 2;
}
