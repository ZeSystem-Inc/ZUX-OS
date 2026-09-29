#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

static GtkWidget *progress_bar;
static GtkWidget *status_label;
static GtkWidget *start_button;

static const char* detect_target_disk(void) {
    static char disk_path[128];
    const char *candidates[] = {"sda", "sdb", "nvme0n1", "vda"};
    
    for (size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); i++) {
        char sys_path[256];
        snprintf(sys_path, sizeof(sys_path), "/sys/block/%s", candidates[i]);
        if (access(sys_path, F_OK) == 0) {
            snprintf(disk_path, sizeof(disk_path), "/dev/%s", candidates[i]);
            return disk_path;
        }
    }

    DIR *d = opendir("/sys/block");
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.' || strncmp(dir->d_name, "loop", 4) == 0 || strncmp(dir->d_name, "sr", 2) == 0)
                continue;
            snprintf(disk_path, sizeof(disk_path), "/dev/%s", dir->d_name);
            closedir(d);
            return disk_path;
        }
        closedir(d);
    }
    return NULL;
}

static void update_status(const char *text, double fraction) {
    gtk_label_set_text(GTK_LABEL(status_label), text);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), fraction);
    while (gtk_events_pending()) gtk_main_iteration();
}

static void on_start_clicked(GtkWidget *widget, gpointer data) {
    gtk_widget_set_sensitive(start_button, FALSE);
    
    const char *target_dev = detect_target_disk();
    if (!target_dev) {
        update_status("Hata: Uygun depolama diski bulunamadi!", 0.0);
        return;
    }

    char status_msg[256];
    snprintf(status_msg, sizeof(status_msg), "Disk otomatik olarak hazirlaniyor: %s", target_dev);
    update_status(status_msg, 0.1);

    char sfdisk_cmd[512];
    snprintf(sfdisk_cmd, sizeof(sfdisk_cmd),
             "echo 'label: dos' | sfdisk %s >/dev/null 2>&1 && "
             "echo ',,L,*' | sfdisk %s >/dev/null 2>&1",
             target_dev, target_dev);
    
    if (system(sfdisk_cmd) != 0) {
        update_status("Hata: Disk otomatik bolumlenemedi!", 0.2);
        return;
    }

    char part_dev[128];
    if (strstr(target_dev, "nvme") != NULL) {
        snprintf(part_dev, sizeof(part_dev), "%sp1", target_dev);
    } else {
        snprintf(part_dev, sizeof(part_dev), "%s1", target_dev);
    }

    sleep(2);
    (void)system("udevadm trigger 2>/dev/null || mdev -s 2>/dev/null");

    update_status("Disk bicimlendiriliyor (ext4)...", 0.35);
    char mkfs_cmd[256];
    snprintf(mkfs_cmd, sizeof(mkfs_cmd), "mkfs.ext4 -F %s >/dev/null 2>&1", part_dev);
    if (system(mkfs_cmd) != 0) {
        update_status("Hata: Disk ext4 olarak bicimlendirilemedi!", 0.4);
        return;
    }

    update_status("Hedef disk baglaniyor...", 0.5);
    (void)system("mkdir -p /mnt/target");
    char mount_cmd[256];
    snprintf(mount_cmd, sizeof(mount_cmd), "mount %s /mnt/target", part_dev);
    if (system(mount_cmd) != 0) {
        update_status("Hata: Hedef disk baglanamadi!", 0.55);
        return;
    }

    update_status("Sistem dosyalari yukleniyor...", 0.7);
    (void)system("cp -a /rootfs/* /mnt/target/ 2>/dev/null");

    update_status("Bootloader (GRUB) yapilandiriliyor...", 0.85);
    char grub_cmd[512];
    snprintf(grub_cmd, sizeof(grub_cmd),
             "grub-install --target=i386-pc --boot-directory=/mnt/target/boot %s >/dev/null 2>&1 && "
             "chroot /mnt/target update-grub >/dev/null 2>&1",
             target_dev);
    (void)system(grub_cmd);

    (void)system("umount /mnt/target");
    update_status("Kurulum Tamamlandi! Yeniden baslatabilirsiniz.", 1.0);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "ZUX OS Graphical Installer");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 250);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), "<span size='x-large' weight='bold'>ZUX OS Kurulumu</span>");
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);

    GtkWidget *info_label = gtk_label_new("Sistem otomatik olarak tespit edilen en uygun diske kurulacaktir.");
    gtk_box_pack_start(GTK_BOX(vbox), info_label, FALSE, FALSE, 0);

    progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), progress_bar, FALSE, FALSE, 0);

    status_label = gtk_label_new("Hazir.");
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 0);

    start_button = gtk_button_new_with_label("Kurulumu Baslat");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), start_button, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
