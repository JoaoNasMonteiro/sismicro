#ifndef USART_H
#define USART_H

#define F_CPU 16000000UL
#define BAUD 9600

#include <stdint.h>

typedef struct pacote {
  uint8_t nível;
  uint8_t ligar;      // fração de 255 para a qual a bomba liga
  uint8_t amostragem; // taxa de amostragem. Modos fixos 0 - y
  uint8_t flags;      // top 2 bits para flag, LF00 0000
} pacote_t;

// TODO: adicionar as assnaturas das funções aqui

#endif /* end of include guard: USART_H */
