#ifndef BRISA_SETTINGS_H
#define BRISA_SETTINGS_H

/* Arquivo chave=valor em Brisa/settings.conf, lido também pelos scripts. */
void settings_load(const char *path);
const char *settings_get(const char *key, const char *def);
int settings_get_int(const char *key, int def);
void settings_set(const char *key, const char *val);
void settings_set_int(const char *key, int val);
int settings_save(void);

#endif
