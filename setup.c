#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    char disk[64];

    printf("=========================================\n");
    printf("     ZUX OS (Debian Base) Installer      \n");
    printf("=========================================\n\n");

    printf("[*] Mevcut Depolama Cihazlari:\n");
    (void)system("lsblk -d -n -o NAME,SIZE,MODEL 2>/dev/null || fdisk -l 2>/dev/null");
    
    printf("\nKurulum yapilacak diski girin (ornek: sda veya nvme0n1): ");
    fflush(stdout);

    if (scanf("%63s", disk) != 1) {
        printf("[!] Hatali girdi.\n");
        return 1;
    }

    char target_dev[128];
    snprintf(target_dev, sizeof(target_dev), "/dev/%s", disk);

    printf("\n[!] DIKKAT: %s uzerindeki TUM VERILER SILINECEK!\n", target_dev);
    printf("Devam etmek istiyor musunuz? (y/n): ");
    fflush(stdout);

    char confirm[8];
    if (scanf("%7s", confirm) != 1 || (confirm[0] != 'y' && confirm[0] != 'Y')) {
        printf("[*] Kurulum iptal edildi.\n");
        return 0;
    }

    printf("\n[*] %s diski MBR olarak bolumleniyor...\n", target_dev);
    
    char sfdisk_cmd[512];
    snprintf(sfdisk_cmd, sizeof(sfdisk_cmd), 
             "echo 'label: dos' | sfdisk %s >/dev/null 2>&1 && "
             "echo ',,L,*' | sfdisk %s >/dev/null 2>&1", 
             target_dev, target_dev);
    
    if (system(sfdisk_cmd) != 0) {
        printf("[!] Disk bolumleme basarisiz oldu.\n");
        return 1;
    }

    char part_dev[128];
    if (strstr(disk, "nvme") != NULL) {
        snprintf(part_dev, sizeof(part_dev), "/dev/%sp1", disk);
    } else {
        snprintf(part_dev, sizeof(part_dev), "/dev/%s1", disk);
    }

    sleep(2);
    (void)system("mdev -s 2>/dev/null || udevadm trigger 2>/dev/null");

    printf("[*] %s ext4 olarak bicimlendiriliyor...\n", part_dev);
    
    char mkfs_cmd[256];
    snprintf(mkfs_cmd, sizeof(mkfs_cmd), "mkfs.ext4 -F %s >/dev/null 2>&1", part_dev);
    
    if (system(mkfs_cmd) != 0) {
        printf("[!] Disk bicimlendirme basarisiz oldu.\n");
        return 1;
    }

    printf("[*] Hedef disk mount ediliyor...\n");
    (void)system("mkdir -p /mnt/target");
    
    char mount_cmd[256];
    snprintf(mount_cmd, sizeof(mount_cmd), "mount %s /mnt/target", part_dev);
    
    if (system(mount_cmd) != 0) {
        printf("[!] Hedef disk mount edilemedi.\n");
        return 1;
    }

    printf("[*] Debian tabanli ZUX OS dosyalari aktariliyor...\n");
    if (system("cp -a /rootfs/* /mnt/target/ 2>/dev/null") != 0) {
        printf("[!] Dosya kopyalamada bazi uyarilar olustu ancak devam ediliyor...\n");
    }

    printf("[*] Hedef sisteme GRUB Bootloader kuruluyor...\n");
    char grub_cmd[512];
    snprintf(grub_cmd, sizeof(grub_cmd),
             "grub-install --target=i386-pc --boot-directory=/mnt/target/boot %s >/dev/null 2>&1 && "
             "chroot /mnt/target update-grub >/dev/null 2>&1",
             target_dev);
    (void)system(grub_cmd);

    printf("[*] Disk unmount ediliyor...\n");
    (void)system("umount /mnt/target");

    printf("\n[+] ZUX OS Debian Tabanli Kurulum Basariyla Tamamlandi!\n");
    printf("[*] Sistemi 'reboot' komutu ile yeniden baslatabilirsiniz.\n");
    
    return 0;
}
