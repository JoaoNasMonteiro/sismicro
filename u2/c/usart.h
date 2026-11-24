#ifndef USART_H
#define USART_H

#define F_CPU 16000000UL
#define BAUD 9600

#include <stdint.h>

typedef struct pacote {
  uint8_t nivel;
  uint8_t ligar;      // Limite inferior recebido do computador
  uint8_t amostragem; // Campo usado pelo firmware como limite superior recebido
  uint8_t flags;      // Bit 7: bomba ligada; bit 6: falha; bits inferiores não usados
} pacote_t;

extern volatile uint8_t flag_pacote_recebido;
extern volatile pacote_t pacote_rx;

void usart_init(void);
void usart_transmit(uint8_t byte);
void usart_print(const char *str);
void usart_enviar_pacote(uint8_t nivel, uint8_t ligar, uint8_t amostragem,
                         uint8_t flags);

#endif /* end of include guard: USART_H */
