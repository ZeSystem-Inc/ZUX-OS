#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    (void)setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);

    mkdir("/proc", 0755);
    mkdir("/sys", 0755);
    mkdir("/dev", 0755);
    mkdir("/mnt", 0755);

    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);

    (void)system("echo 1 > /proc/sys/kernel/printk");

    int fd = open("/dev/tty1", O_RDWR);
    if (fd < 0) fd = open("/dev/console", O_RDWR);
    if (fd >= 0) {
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        if (fd > 2) close(fd);
    }

    printf("\033[H\033[J");
    printf("===============================================\n");
    printf("       ZUX OS Debian Offline Installer         \n");
    printf("===============================================\n\n");

    (void)system("mdev -s 2>/dev/null || udevadm trigger 2>/dev/null");

    if (access("/installer/setup.elf", X_OK) == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            execl("/installer/setup.elf", "/installer/setup.elf", NULL);
            _exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        }
    }

    while (1) {
        pid_t pid = fork();
        if (pid == 0) {
            execl("/bin/sh", "sh", NULL);
            _exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        }
        sleep(1);
    }
    return 0;
}
