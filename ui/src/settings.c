#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "settings.h"

#define MAX_KEYS 48

static struct {
    char key[32];
    char val[128];
} kv[MAX_KEYS];
static int nkv;
static char file[512];

void settings_load(const char *path)
{
    snprintf(file, sizeof file, "%s", path);
    nkv = 0;
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *eq = strchr(line, '=');
        if (line[0] == '#' || !eq)
            continue;
        *eq = 0;
        settings_set(line, eq + 1);
    }
    fclose(f);
}

const char *settings_get(const char *key, const char *def)
{
    for (int i = 0; i < nkv; i++)
        if (strcmp(kv[i].key, key) == 0)
            return kv[i].val;
    return def;
}

int settings_get_int(const char *key, int def)
{
    const char *v = settings_get(key, NULL);
    return v ? atoi(v) : def;
}

void settings_set(const char *key, const char *val)
{
    int i;
    for (i = 0; i < nkv; i++)
        if (strcmp(kv[i].key, key) == 0)
            break;
    if (i == nkv) {
        if (nkv == MAX_KEYS)
            return;
        nkv++;
        snprintf(kv[i].key, sizeof kv[i].key, "%s", key);
    }
    snprintf(kv[i].val, sizeof kv[i].val, "%s", val);
}

void settings_set_int(const char *key, int val)
{
    char buf[16];
    snprintf(buf, sizeof buf, "%d", val);
    settings_set(key, buf);
}

/* Grava em arquivo temporário e renomeia: uma queda de energia no meio
 * da escrita não corrompe as configurações. */
int settings_save(void)
{
    char tmp[600];
    snprintf(tmp, sizeof tmp, "%s.tmp", file);
    FILE *f = fopen(tmp, "w");
    if (!f)
        return -1;
    for (int i = 0; i < nkv; i++)
        fprintf(f, "%s=%s\n", kv[i].key, kv[i].val);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    return rename(tmp, file);
}
