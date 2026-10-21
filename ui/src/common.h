#ifndef BRISA_COMMON_H
#define BRISA_COMMON_H

#include <stddef.h>
#include <stdint.h>

#define BRISA_VERSION "0.1.0"

/* Tela em memória, sempre 0xAARRGGBB, com w pixels por linha. */
typedef struct {
    int w, h;
    uint32_t *px;
} canvas_t;

/* Botões lógicos da interface. Os códigos físicos ficam em input.conf. */
enum {
    IN_NONE = 0,
    IN_UP, IN_DOWN, IN_LEFT, IN_RIGHT,
    IN_A, IN_B, IN_X, IN_Y,
    IN_L1, IN_R1, IN_L2, IN_R2,
    IN_START, IN_SELECT, IN_MENU,
    IN_POWER, IN_VOLUP, IN_VOLDOWN,
    IN_COUNT
};

typedef enum { UEV_NONE, UEV_INPUT, UEV_QUIT } uev_kind_t;

typedef struct {
    uev_kind_t kind;
    int btn;      /* IN_*, ou IN_NONE se o código não está mapeado */
    int pressed;  /* 1 = apertou, 0 = soltou */
    /* evento bruto, mostrado na tela de teste de controles */
    int raw_type, raw_code, raw_value;
    char raw_dev[32];
} uev_t;

const char *in_name(int btn);
int in_from_name(const char *name);
uint64_t now_ms(void);

#endif
