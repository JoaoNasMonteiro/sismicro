#ifndef USART_H
#define USART_H

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef BAUD
#define BAUD 9600
#endif

#include <stdint.h>

/*
 * Bits do campo `flags` (formato LF00 0000 nos 2 bits mais significativos):
 * - Bit 7 (FLAG_LIGADA): 1 = bomba ligada, 0 = bomba desligada
 * - Bit 6 (FLAG_FALHA):  1 = falha detectada, 0 = operação normal
 * - Bit 5 (FLAG_FORCAR): 1 = comando do computador para forçar estado da bomba
 *                        (deve ser avaliado com ressalva de segurança)
 */
#define FLAG_LIGADA (1 << 7)
#define FLAG_FALHA  (1 << 6)
#define FLAG_FORCAR (1 << 5)

/* Limites operacionais padrão do reservatório (capacidade fixa de 255 L) */
#define CAPACIDADE_MAXIMA     255U
#define NIVEL_MAX_SEGURANCA   245U
#define NIVEL_LIGAR_PADRAO    64U
#define AMOSTRAGEM_MODO_MAX   3U

/* Tamanho dos buffers circulares de transmissão e recepção (potência de 2) */
#define USART_TX_BUFFER_SIZE  32U
#define USART_RX_BUFFER_SIZE  32U

typedef struct pacote {
  uint8_t nivel;      // Nível atual da água no reservatório (0 a 255 L)
  uint8_t ligar;      // Fração de 255 para a qual a bomba liga
  uint8_t amostragem; // Taxa de amostragem. Modos fixos 0 - y
  uint8_t flags;      // Top 2 bits para flag: LF00 0000 (Bit 7 = L, Bit 6 = F)
} pacote_t;

/* Inicializa o periférico USART0 com interrupções de TX e RX habilitadas */
void usart_init(void);

/* Enfileira um byte para transmissão assíncrona via interrupção (UDRE0) */
void usart_transmit(uint8_t data);

/* Enfileira os 4 bytes de um pacote_t para transmissão via interrupção */
void usart_transmit_pacote(const pacote_t *pkt);

/* Envia uma string terminada em '\0' de forma não-bloqueante via buffer TX */
void usart_print(const char *str);

/* Envia a representação textual formatada de um pacote_t (útil para debug) */
void usart_print_pacote(const pacote_t *pkt);

/* Retorna a quantidade de bytes disponíveis no buffer de recepção */
uint8_t usart_available(void);

/* Lê um byte do buffer de recepção (retorna 1 se leu com sucesso, 0 se vazio) */
uint8_t usart_receive(uint8_t *data);

/* Retorna 1 se um pacote_t completo (4 bytes) foi recebido do computador */
uint8_t usart_pacote_disponivel(void);

/*
 * Copia o último pacote recebido para `pkt` de forma atômica.
 * Retorna 1 se havia pacote novo disponível, ou 0 caso contrário.
 */
uint8_t usart_receive_pacote(pacote_t *pkt);

/*
 * Aplica os comandos recebidos do computador (`cmd`) sobre o estado atual da
 * placa (`estado`), atualizando os parâmetros (`ligar`, `amostragem`) e
 * tratando o acionamento/desligamento forçado com ressalvas de segurança.
 */
void usart_aplicar_comando(const pacote_t *cmd, pacote_t *estado);

/*
 * Verifica condição de falha: se a bomba estiver ligada e o nível do
 * reservatório estiver diminuindo (ou não estiver subindo), seta FLAG_FALHA.
 */
void usart_verificar_falha(pacote_t *estado, uint8_t nivel_anterior);

#endif /* end of include guard: USART_H */
