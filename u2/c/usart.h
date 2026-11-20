#ifndef USART_H
#define USART_H

#define F_CPU 16000000UL
#define BAUD 9600

#include <stdint.h>

typedef struct pacote {
  uint8_t nivel;
  uint8_t ligar;      // fração de 255 para a qual a bomba liga
  uint8_t amostragem; // taxa de amostragem. Modos fixos 0 - y
  uint8_t flags;      // top 2 bits para flag, LF00 0000
} pacote_t;

extern volatile uint8_t flag_pacote_recebido;
extern volatile pacote_t pacote_rx;

void usart_init(void);
void usart_transmit(uint8_t byte);
void usart_print(const char *str);
void usart_enviar_pacote(uint8_t nivel, uint8_t ligar, uint8_t amostragem,
                         uint8_t flags);

#endif /* end of include guard: USART_H */
