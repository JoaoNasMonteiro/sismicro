#include "./usart.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>

/*
 * Mapeamento de registradores de hardware do ATmega328P utilizados na USART0:
 * - UBRR0H / UBRR0L : Registradores de taxa de transmissão (Baud Rate)
 * - UCSR0A          : Status e controle A (RXC0, TXC0, UDRE0, FE0, DOR0, U2X0)
 * - UCSR0B          : Controle B (RXCIE0, UDRIE0, RXEN0, TXEN0)
 * - UCSR0C          : Controle C (UCSZ01, UCSZ00 -> formato 8N1)
 * - UDR0            : Registrador de dados de E/S da USART0
 */

#define TX_BUFFER_MASK (USART_TX_BUFFER_SIZE - 1U)
#define RX_BUFFER_MASK (USART_RX_BUFFER_SIZE - 1U)

/* Buffers circulares voláteis para operação orientada a interrupção */
static volatile uint8_t tx_buffer[USART_TX_BUFFER_SIZE];
static volatile uint8_t tx_head = 0;
static volatile uint8_t tx_tail = 0;

static volatile uint8_t rx_buffer[USART_RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

/* Montagem automática de pacotes de 4 bytes (pacote_t) na interrupção de RX */
static volatile uint8_t rx_pkt_bytes[sizeof(pacote_t)];
static volatile uint8_t rx_pkt_idx = 0;
static volatile pacote_t rx_ultimo_pacote;
static volatile uint8_t rx_pacote_pronto = 0;

/*
 * ISR de Registrador de Dados Vazio (UDRE0):
 * Disparada automaticamente pelo hardware enquanto houver bytes na fila de TX.
 */
ISR(USART_UDRE_vect) {
  if (tx_head != tx_tail) {
    UDR0 = tx_buffer[tx_tail];
    tx_tail = (tx_tail + 1U) & TX_BUFFER_MASK;
  } else {
    /* Fila vazia: desabilita a interrupção UDRE0 até novo envio */
    UCSR0B &= ~(1 << UDRIE0);
  }
}

/*
 * ISR de Recepção Completa (RXC0):
 * Disparada automaticamente quando um novo byte chega em UDR0.
 */
ISR(USART_RX_vect) {
  uint8_t status = UCSR0A;
  uint8_t data = UDR0;

  /* Descarta bytes com erro de frame (FE0) ou overrun de hardware (DOR0) */
  if (status & ((1 << FE0) | (1 << DOR0))) {
    rx_pkt_idx = 0;
    return;
  }

  /* Armazena no buffer circular de bytes brutos */
  uint8_t next_head = (rx_head + 1U) & RX_BUFFER_MASK;
  if (next_head != rx_tail) {
    rx_buffer[rx_head] = data;
    rx_head = next_head;
  }

  /* Monta a estrutura pacote_t (4 bytes: nivel, ligar, amostragem, flags) */
  rx_pkt_bytes[rx_pkt_idx++] = data;
  if (rx_pkt_idx >= sizeof(pacote_t)) {
    rx_ultimo_pacote.nivel = rx_pkt_bytes[0];
    rx_ultimo_pacote.ligar = rx_pkt_bytes[1];
    rx_ultimo_pacote.amostragem = rx_pkt_bytes[2];
    rx_ultimo_pacote.flags = rx_pkt_bytes[3];
    rx_pacote_pronto = 1;
    rx_pkt_idx = 0;
  }
}

void usart_init(void) {
  uint8_t sreg = SREG;
  cli();

  tx_head = 0;
  tx_tail = 0;
  rx_head = 0;
  rx_tail = 0;
  rx_pkt_idx = 0;
  rx_pacote_pronto = 0;

  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

#if USE_2X
  UCSR0A |= (1 << U2X0);
#else
  UCSR0A &= ~(1 << U2X0);
#endif

  /*
   * Habilita receptor (RXEN0), transmissor (TXEN0) e interrupção de recepção
   * completa (RXCIE0). A interrupção de transmissão (UDRIE0) é habilitada sob
   * demanda quando há dados no buffer de envio.
   */
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

  /* Formato do frame: 8 bits de dados, 1 stop bit, sem paridade (8N1) */
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

  SREG = sreg;
  sei();
}

void usart_transmit(uint8_t data) {
  uint8_t next_head = (tx_head + 1U) & TX_BUFFER_MASK;

  /* Caso o buffer circular esteja cheio, aguarda a ISR liberar espaço */
  while (next_head == tx_tail) {
  }

  uint8_t sreg = SREG;
  cli();
  tx_buffer[tx_head] = data;
  tx_head = next_head;
  /* Habilita a interrupção de registrador vazio para iniciar/continuar envio */
  UCSR0B |= (1 << UDRIE0);
  SREG = sreg;
}

void usart_transmit_pacote(const pacote_t *pkt) {
  if (!pkt) {
    return;
  }
  usart_transmit(pkt->nivel);
  usart_transmit(pkt->ligar);
  usart_transmit(pkt->amostragem);
  usart_transmit(pkt->flags);
}

void usart_print(const char *str) {
  if (!str) {
    return;
  }
  while (*str) {
    usart_transmit((uint8_t)*str++);
  }
}

static void usart_print_uint8(uint8_t val) {
  char buf[4];
  uint8_t i = 0;

  if (val == 0) {
    usart_transmit('0');
    return;
  }
  while (val > 0) {
    buf[i++] = (char)('0' + (val % 10U));
    val /= 10U;
  }
  while (i > 0) {
    usart_transmit((uint8_t)buf[--i]);
  }
}

void usart_print_pacote(const pacote_t *pkt) {
  if (!pkt) {
    return;
  }
  usart_print("Nivel: ");
  usart_print_uint8(pkt->nivel);
  usart_print("L | Ligar: ");
  usart_print_uint8(pkt->ligar);
  usart_print(" | Amostragem: ");
  usart_print_uint8(pkt->amostragem);
  usart_print(" | Bomba: ");
  usart_print((pkt->flags & FLAG_LIGADA) ? "ON" : "OFF");
  usart_print(" | Falha: ");
  usart_print((pkt->flags & FLAG_FALHA) ? "SIM\r\n" : "NAO\r\n");
}

uint8_t usart_available(void) { return (rx_head - rx_tail) & RX_BUFFER_MASK; }

uint8_t usart_receive(uint8_t *data) {
  if (rx_head == rx_tail) {
    return 0;
  }
  uint8_t sreg = SREG;
  cli();
  *data = rx_buffer[rx_tail];
  rx_tail = (rx_tail + 1U) & RX_BUFFER_MASK;
  SREG = sreg;
  return 1;
}

uint8_t usart_pacote_disponivel(void) { return rx_pacote_pronto; }

uint8_t usart_receive_pacote(pacote_t *pkt) {
  if (!pkt || !rx_pacote_pronto) {
    return 0;
  }

  uint8_t sreg = SREG;
  cli();
  pkt->nivel = rx_ultimo_pacote.nivel;
  pkt->ligar = rx_ultimo_pacote.ligar;
  pkt->amostragem = rx_ultimo_pacote.amostragem;
  pkt->flags = rx_ultimo_pacote.flags;
  rx_pacote_pronto = 0;
  SREG = sreg;

  return 1;
}

void usart_aplicar_comando(const pacote_t *cmd, pacote_t *estado) {
  if (!cmd || !estado) {
    return;
  }

  /* Atualiza o limiar de acionamento (respeitando o teto de segurança) */
  if (cmd->ligar < NIVEL_MAX_SEGURANCA) {
    estado->ligar = cmd->ligar;
  } else {
    estado->ligar = NIVEL_MAX_SEGURANCA;
  }

  /* Atualiza o modo da taxa de amostragem (0 até AMOSTRAGEM_MODO_MAX) */
  if (cmd->amostragem <= AMOSTRAGEM_MODO_MAX) {
    estado->amostragem = cmd->amostragem;
  }

  /*
   * Verifica solicitação de forçar ligar/desligar (com ressalvas):
   * - Não força LIGAR se houver falha detectada ou se o reservatório já
   *   estiver no nível máximo de segurança (evita transbordamento).
   * - Não força DESLIGAR se o nível estiver abaixo do mínimo configurado em
   *   `ligar` (evita falta de água no reservatório).
   */
  if (cmd->flags & FLAG_FORCAR) {
    if (cmd->flags & FLAG_LIGADA) {
      if (!(estado->flags & FLAG_FALHA) &&
          (estado->nivel < NIVEL_MAX_SEGURANCA)) {
        estado->flags |= FLAG_LIGADA;
      }
    } else {
      if (estado->nivel >= estado->ligar) {
        estado->flags &= ~FLAG_LIGADA;
      }
    }
  }
}

void usart_verificar_falha(pacote_t *estado, uint8_t nivel_anterior) {
  if (!estado) {
    return;
  }

  /*
   * Identificação de falha conforme especificação:
   * Se a bomba estiver ligada e o nível do reservatório estiver diminuindo
   * (ou não apresentar elevação), sinaliza o bit FLAG_FALHA.
   */
  if (estado->flags & FLAG_LIGADA) {
    if (estado->nivel <= nivel_anterior) {
      estado->flags |= FLAG_FALHA;
    } else {
      estado->flags &= ~FLAG_FALHA;
    }
  }
}

// Exemplo de uso na rotina principal:
// int main(void) {
//   pacote_t estado = {
//       .nivel = 120,
//       .ligar = NIVEL_LIGAR_PADRAO,
//       .amostragem = 1,
//       .flags = 0,
//   };
//   pacote_t cmd;
//
//   usart_init();
//
//   while (1) {
//     if (usart_receive_pacote(&cmd)) {
//       usart_aplicar_comando(&cmd, &estado);
//     }
//     usart_transmit_pacote(&estado);
//     _delay_ms(1000);
//   }
//   return 0;
// }
