#include "./usart.h"

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>

// Define o clock e o baud rate ANTES de incluir os headers

// 1. Função para inicializar o USART
void usart_init(void) {
  // Configura os registradores de Baud Rate usando as macros da setbaud.h
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

#if USE_2X
  UCSR0A |=
      (1 << U2X0); // Ativa velocidade dupla se a macro determinar necessário
#else
  UCSR0A &= ~(1 << U2X0);
#endif

  // Habilita o transmissor (TXEN0)
  UCSR0B = (1 << TXEN0);

  // Configura o formato do frame: 8 bits de dados (UCSZ01 e UCSZ00)
  // O padrão de reset do ATmega328 já é 8N1, mas explicitar é uma boa prática
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// 2. Função para transmitir um único caractere
void usart_transmit(unsigned char data) {
  // Espera o buffer de transmissão ficar vazio (flag UDRE0)
  // Isso é chamado de "Polling". O código trava aqui até o hardware liberar.
  while (!(UCSR0A & (1 << UDRE0)))
    ;
  // Coloca os dados no registrador de dados para envio
  UDR0 = data;
}

// 3. Função auxiliar para enviar uma string completa
void usart_print(const char *str) {
  while (*str) {
    usart_transmit(*str++);
  }
}

int main(void) {
  usart_init();

  while (1) {
    usart_print("Ola do ATmega328!\r\n");

    // Aguarda 1 segundo antes de enviar de novo
    _delay_ms(1000);
  }

  return 0;
}
