#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    (void)setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin:/usr/bin/X11", 1);
    (void)setenv("DISPLAY", ":0", 1);

    mkdir("/proc", 0755);
    mkdir("/sys", 0755);
    mkdir("/dev", 0755);
    mkdir("/mnt", 0755);

    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);

    (void)system("udevadm trigger 2>/dev/null || mdev -s 2>/dev/null");

    pid_t xorg_pid = fork();
    if (xorg_pid == 0) {
        execl("/usr/bin/Xorg", "Xorg", ":0", "-nolisten", "tcp", "vt1", NULL);
        _exit(1);
    }

    sleep(3);

    pid_t wm_pid = fork();
    if (wm_pid == 0) {
        execl("/usr/bin/openbox", "openbox", NULL);
        _exit(1);
    }

    sleep(1);

    if (access("/installer/setup.elf", X_OK) == 0) {
        pid_t app_pid = fork();
        if (app_pid == 0) {
            execl("/installer/setup.elf", "/installer/setup.elf", NULL);
            _exit(1);
        } else if (app_pid > 0) {
            int status;
            waitpid(app_pid, &status, 0);
        }
    }

    while (1) {
        sleep(10);
    }
    return 0;
}
