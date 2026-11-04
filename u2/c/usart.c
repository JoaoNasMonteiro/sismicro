#include "./usart.h"

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>

// TODO: geral- modificar para basear em interrupt em vés de busywait
// TODO: definir quais registros vão ser usados para cada variável na placa

// TODO: configuração para enviar e receber também
void usart_init(void) {
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

#if USE_2X
  UCSR0A |= (1 << U2X0);
#else
  UCSR0A &= ~(1 << U2X0);
#endif

  // Habilita o transmissor (TXEN0)
  UCSR0B = (1 << TXEN0);

  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// TODO: adaptart para funcionar com o formato de dados customizado.
void usart_transmit(unsigned char data) {
  // Espera o buffer de transmissão ficar vazio (flag UDRE0)
  while (!(UCSR0A & (1 << UDRE0))) {
  }
  // Coloca os dados no registrador de dados para envio
  UDR0 = data;
}

// TODO: atualizar para aceitar o formato de dados customizado.
void usart_print(const char *str) {
  while (*str) {
    usart_transmit(*str++);
  }
}

// TODO: adicionar função apra receber dados em vés de somente enviar

//
// int main(void) {
//   usart_init();
//
//   while (1) {
//     usart_print("Ola do ATmega328!\r\n");
//
//     // Aguarda 1 segundo antes de enviar de novo
//     _delay_ms(1000);
//   }
//
//   return 0;
// }
