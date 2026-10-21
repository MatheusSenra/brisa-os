#ifndef BRISA_SYSINFO_H
#define BRISA_SYSINFO_H

#include <stddef.h>

/* Porcentagem da bateria, ou -1 se não houver; *charging = carregando. */
int sys_battery(int *charging);
/* IPv4 da primeira interface que não seja loopback; 0 se não houver. */
int sys_ip(char *buf, size_t n);
/* Brilho em níveis de 1 a 10; devolve 0 se o display não permite ajuste. */
int bright_available(void);
void bright_set(int level);
/* Espaço livre em MB no sistema de arquivos de path. */
long sys_free_mb(const char *path);
void sys_kernel(char *buf, size_t n);
long sys_ram_mb(void);

#endif
