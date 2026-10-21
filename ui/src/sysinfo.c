#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "sysinfo.h"

/* ioctl do driver de display da Allwinner (sunxi disp2), usado quando o
 * kernel stock não expõe /sys/class/backlight. */
#define DISP_LCD_SET_BRIGHTNESS 0x102

static int read_str(const char *path, char *buf, size_t n)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    if (!fgets(buf, (int)n, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    buf[strcspn(buf, "\r\n")] = 0;
    return 0;
}

int sys_battery(int *charging)
{
    *charging = 0;
    DIR *d = opendir("/sys/class/power_supply");
    if (!d)
        return -1;
    struct dirent *e;
    int pct = -1;
    while ((e = readdir(d)) && pct < 0) {
        if (e->d_name[0] == '.')
            continue;
        char p[300], v[64];
        snprintf(p, sizeof p, "/sys/class/power_supply/%s/type", e->d_name);
        if (read_str(p, v, sizeof v) < 0 || strcmp(v, "Battery") != 0)
            continue;
        snprintf(p, sizeof p, "/sys/class/power_supply/%s/capacity", e->d_name);
        if (read_str(p, v, sizeof v) == 0)
            pct = atoi(v);
        snprintf(p, sizeof p, "/sys/class/power_supply/%s/status", e->d_name);
        if (read_str(p, v, sizeof v) == 0)
            *charging = strcmp(v, "Charging") == 0 || strcmp(v, "Full") == 0;
    }
    closedir(d);
    return pct;
}

int sys_ip(char *buf, size_t n)
{
    struct ifaddrs *ifa, *i;
    if (getifaddrs(&ifa) < 0)
        return 0;
    int found = 0;
    for (i = ifa; i && !found; i = i->ifa_next) {
        if (!i->ifa_addr || i->ifa_addr->sa_family != AF_INET || strcmp(i->ifa_name, "lo") == 0)
            continue;
        inet_ntop(AF_INET, &((struct sockaddr_in *)i->ifa_addr)->sin_addr, buf, (socklen_t)n);
        found = 1;
    }
    freeifaddrs(ifa);
    return found;
}

static int backlight_dir(char *out, size_t n)
{
    DIR *d = opendir("/sys/class/backlight");
    if (!d)
        return 0;
    struct dirent *e;
    int ok = 0;
    while ((e = readdir(d)))
        if (e->d_name[0] != '.') {
            snprintf(out, n, "/sys/class/backlight/%s", e->d_name);
            ok = 1;
            break;
        }
    closedir(d);
    return ok;
}

int bright_available(void)
{
    char dir[300];
    return backlight_dir(dir, sizeof dir) || access("/dev/disp", R_OK | W_OK) == 0;
}

void bright_set(int level)
{
    if (level < 1) level = 1;
    if (level > 10) level = 10;
    char dir[300], p[340], v[32];
    if (backlight_dir(dir, sizeof dir)) {
        snprintf(p, sizeof p, "%s/max_brightness", dir);
        int max = read_str(p, v, sizeof v) == 0 ? atoi(v) : 255;
        /* nível 1 ainda visível: escala de 8% a 100% */
        int val = max * (8 + (level - 1) * 92 / 9) / 100;
        snprintf(p, sizeof p, "%s/brightness", dir);
        FILE *f = fopen(p, "w");
        if (f) {
            fprintf(f, "%d\n", val);
            fclose(f);
        }
        return;
    }
    int fd = open("/dev/disp", O_RDWR);
    if (fd < 0)
        return;
    unsigned long args[4] = {0, (unsigned long)(255 * (8 + (level - 1) * 92 / 9) / 100), 0, 0};
    ioctl(fd, DISP_LCD_SET_BRIGHTNESS, args);
    close(fd);
}

long sys_free_mb(const char *path)
{
    struct statvfs s;
    if (statvfs(path, &s) < 0)
        return -1;
    return (long)((unsigned long long)s.f_bavail * s.f_frsize / (1024 * 1024));
}

void sys_kernel(char *buf, size_t n)
{
    struct utsname u;
    if (uname(&u) == 0)
        snprintf(buf, n, "%s %s", u.sysname, u.release);
    else
        snprintf(buf, n, "?");
}

long sys_ram_mb(void)
{
    struct sysinfo si;
    if (sysinfo(&si) < 0)
        return -1;
    return (long)((unsigned long long)si.totalram * si.mem_unit / (1024 * 1024));
}
