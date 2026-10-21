#ifndef BRISA_SYSTEMS_H
#define BRISA_SYSTEMS_H

#define MAX_SYSTEMS 48

/* Uma linha de systems.conf: id|Nome|pastas|extensões|core|pesado */
typedef struct {
    char id[16];
    char name[48];
    char folders[128]; /* separadas por vírgula; a primeira é a padrão */
    char exts[128];
    char core[64];
    int heavy;
    int count; /* jogos encontrados */
} system_t;

typedef struct {
    char *name; /* nome exibido (arquivo sem extensão) */
    char *path;
    int sys;    /* índice em systems[] */
} game_t;

int systems_load(const char *conf, system_t *out, int max);
void systems_count(system_t *sys, int n, const char *roms);
/* Lista os jogos do sistema em todas as suas pastas, em ordem alfabética. */
int games_list(const system_t *sys, int sysidx, const char *roms, game_t **out);
void games_free(game_t *g, int n);
/* "Super Mario World (USA).sfc" -> "Super Mario World (USA)" */
void game_display_name(const char *path, char *out, int n);

#endif
