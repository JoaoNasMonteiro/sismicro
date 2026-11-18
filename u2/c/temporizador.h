#ifndef TEMPORIZADOR_H
#define TEMPORIZADOR_H

#include <stdint.h>

/*
 * Essa flag é "ligada" (vira 1) toda vez que o timer dispara.
 * O main.c pode checar essa flag pra saber que passou o tempo certo.
 *
 * "volatile" avisa o compilador que essa variável muda sozinha
 * (dentro da interrupção), então ele não pode ignorar ela.
 */
extern volatile uint8_t flag_tempo;

/* Configura e liga o Timer1 pra disparar a cada 1 segundo */
void timer_init(void);

#endif
