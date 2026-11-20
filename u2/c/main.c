#include "main.h"
#include <avr/interrupt.h>
#include <avr/io.h>

static uint8_t limite_ligar = 51;     /*20% do nível do tanque*/
static uint8_t limite_desligar = 204; /*80% do nível*/

static uint8_t bomba_ligada = 0;
static uint8_t nivel_anterior = 0;
static uint8_t contador_falha = 0;

void bomba_init(void) {
  DDRB |= (1 << PINB0);   /*Configura o pino 0 de PORTB como saída*/
  PORTB &= ~(1 << PINB0); /*Inicia desligada*/
}

void bomba_ligar(void) {
  PORTB |= (1 << PINB0);
  bomba_ligada = 1;
}

void bomba_desligar(void) {
  PORTB &= ~(1 << PINB0);
  bomba_ligada = 0;
}

int main(void) {
  adc_init();
  timer_init();
  usart_init();
  bomba_init();

  sei();

  while (1) {
    /*O timer já encerrou a contagem*/
    if (flag_tempo) {
      flag_tempo = 0;
    }

    /*flag que indica que o ADC já realizou 1 leitura*/
    if (flag_leitura_pronta) {
      flag_leitura_pronta = 0;
      uint8_t nivel = nivel_atual;

      if (nivel <= limite_ligar) {
        bomba_ligar();
      } else if (nivel >= limite_desligar) {
        bomba_desligar();
      }

      if (bomba_ligada && nivel <= nivel_anterior) {
        contador_falha++;
      } else {
        contador_falha = 0;
      }

      /* Guarda a leitura atual para a próxima comparação */
      nivel_anterior = nivel;

      /* ── Montagem e Envio do Pacote Serial ──
       * Flags (LF00 0000):
       *   Bit 7 (L): 1 = Bomba ligada, 0 = desligada
       *   Bit 6 (F): 1 = Falha detectada (5s sem aumento de nível)
       *   Bit 5 (M): 1 = Modo manual ativo, 0 = automático
       */
      uint8_t flags = 0;
      if (bomba_ligada) {
        flags |= (1 << 7);
      }
      if (contador_falha >= 5) {
        flags |= (1 << 6);
      }

      /* Envia pacote de 4 bytes via USART */
      usart_enviar_pacote(nivel, limite_ligar, limite_desligar, flags);
    }

    if (flag_pacote_recebido) {
      flag_pacote_recebido = 0;
      limite_ligar = pacote_rx.ligar;
      limite_desligar = pacote_rx.amostragem;
    }
  }
  return 0;
}
