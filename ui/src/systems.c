#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "systems.h"

static void field(char **s, char *out, size_t n)
{
    char *end = strchr(*s, '|');
    size_t len = end ? (size_t)(end - *s) : strcspn(*s, "\r\n");
    if (len >= n)
        len = n - 1;
    memcpy(out, *s, len);
    out[len] = 0;
    *s = end ? end + 1 : *s + strlen(*s);
}

int systems_load(const char *conf, system_t *out, int max)
{
    FILE *f = fopen(conf, "r");
    if (!f)
        return 0;
    char line[512];
    int n = 0;
    while (n < max && fgets(line, sizeof line, f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        system_t *s = &out[n];
        char heavy[8];
        char *p = line;
        memset(s, 0, sizeof *s);
        field(&p, s->id, sizeof s->id);
        field(&p, s->name, sizeof s->name);
        field(&p, s->folders, sizeof s->folders);
        field(&p, s->exts, sizeof s->exts);
        field(&p, s->core, sizeof s->core);
        field(&p, heavy, sizeof heavy);
        s->heavy = atoi(heavy);
        if (s->id[0] && s->folders[0] && s->exts[0])
            n++;
    }
    fclose(f);
    return n;
}

static int ext_ok(const system_t *s, const char *file)
{
    const char *dot = strrchr(file, '.');
    if (!dot || !dot[1])
        return 0;
    size_t el = strlen(dot + 1);
    const char *p = s->exts;
    while (*p) {
        size_t len = strcspn(p, ",");
        if (len == el && strncasecmp(p, dot + 1, len) == 0)
            return 1;
        p += len;
        if (*p == ',')
            p++;
    }
    return 0;
}

/* Chama fn para cada ROM do sistema; devolve quantas achou. */
static int scan(const system_t *s, const char *roms, void (*fn)(void *, const char *, const char *), void *ud)
{
    int count = 0;
    const char *p = s->folders;
    while (*p) {
        size_t len = strcspn(p, ",");
        char dir[1024];
        snprintf(dir, sizeof dir, "%s/%.*s", roms, (int)len, p);
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *e;
            while ((e = readdir(d))) {
                /* ignora ocultos e o lixo "._" que o macOS deixa em cartões FAT */
                if (e->d_name[0] == '.' || !ext_ok(s, e->d_name))
                    continue;
                if (e->d_type == DT_DIR)
                    continue;
                count++;
                if (fn)
                    fn(ud, dir, e->d_name);
            }
            closedir(d);
        }
        p += len;
        if (*p == ',')
            p++;
    }
    return count;
}

void systems_count(system_t *sys, int n, const char *roms)
{
    for (int i = 0; i < n; i++)
        sys[i].count = scan(&sys[i], roms, NULL, NULL);
}

struct collect {
    game_t *g;
    int n, cap, sys;
};

static void collect_one(void *ud, const char *dir, const char *file)
{
    struct collect *c = ud;
    if (c->n == c->cap) {
        c->cap = c->cap ? c->cap * 2 : 64;
        c->g = realloc(c->g, (size_t)c->cap * sizeof *c->g);
    }
    char path[1280], name[256];
    snprintf(path, sizeof path, "%s/%s", dir, file);
    game_display_name(file, name, sizeof name);
    c->g[c->n].path = strdup(path);
    c->g[c->n].name = strdup(name);
    c->g[c->n].sys = c->sys;
    c->n++;
}

static int by_name(const void *a, const void *b)
{
    return strcasecmp(((const game_t *)a)->name, ((const game_t *)b)->name);
}

int games_list(const system_t *sys, int sysidx, const char *roms, game_t **out)
{
    struct collect c = {.sys = sysidx};
    scan(sys, roms, collect_one, &c);
    if (c.n > 1)
        qsort(c.g, (size_t)c.n, sizeof *c.g, by_name);
    *out = c.g;
    return c.n;
}

void games_free(game_t *g, int n)
{
    for (int i = 0; i < n; i++) {
        free(g[i].name);
        free(g[i].path);
    }
    free(g);
}

void game_display_name(const char *path, char *out, int n)
{
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    snprintf(out, (size_t)n, "%s", base);
    char *dot = strrchr(out, '.');
    if (dot && dot != out)
        *dot = 0;
}
