#include "./usart.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>

volatile uint8_t flag_pacote_recebido = 0;
volatile pacote_t pacote_rx;
static volatile uint8_t rx_buffer[4];
static volatile uint8_t rx_indice = 0;

void usart_init(void) {
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

#if USE_2X
  UCSR0A |= (1 << U2X0);
#else
  UCSR0A &= ~(1 << U2X0);
#endif

  // Habilita o transmissor (TXEN0)
  UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);

  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void usart_transmit(unsigned char data) {
  // Espera o buffer de transmissão ficar vazio (flag UDRE0)
  while (!(UCSR0A & (1 << UDRE0))) {
  }
  // Coloca os dados no registrador de dados para envio
  UDR0 = data;
}

void usart_print(const char *str) {
  while (*str) {
    usart_transmit(*str++);
  }
}

void usart_enviar_pacote(uint8_t nivel, uint8_t ligar, uint8_t amostragem,
                         uint8_t flags) {
  usart_transmit(nivel);
  usart_transmit(ligar);
  usart_transmit(amostragem);
  usart_transmit(flags);
}

ISR(USART_RX_vect) {
  uint8_t byte_recebido = UDR0; /* Sempre lê UDR0 pra limpar a flag */
  /* Se o main ainda não consumiu o pacote anterior, descarta */
  if (flag_pacote_recebido) {
    return;
  }
  rx_buffer[rx_indice] = byte_recebido;
  rx_indice++;
  /* Quando completou 4 bytes, monta o pacote */
  if (rx_indice >= 4) {
    pacote_rx.nivel = rx_buffer[0];
    pacote_rx.ligar = rx_buffer[1];
    pacote_rx.amostragem = rx_buffer[2];
    pacote_rx.flags = rx_buffer[3];
    flag_pacote_recebido = 1;
    rx_indice = 0;
  }
}
