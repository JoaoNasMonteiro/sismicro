#include <stdint.h>
#include <stdio.h>

typedef uint8_t REG_t;

typedef struct ufix16 {
  REG_t MSB;
  REG_t LSB;
} ufix16_t;

/*
 * ============================================================================
 * ANÁLISE NUMÉRICA E ARQUITETURAL DA IMPLEMENTAÇÃO DA RAIZ QUADRADA
 * ============================================================================
 *
 * 1. FORMATO DE DADOS, LIMITES E OVERFLOW:
 *    - Entrada (Q16.0): Inteiro sem sinal de 16 bits.
 *      Range de Entrada: 0 a 65535. Resolução: 1.
 *    - Saída (Q8.8): Ponto fixo sem sinal, 8 bits inteiros, 8 bits
 * fracionários. Range de Saída: 0.0 a 255.99609375 (0x0000 a 0xFFFF). Resolução
 * da Saída (LSB): 2^-8 = 1/256 = 0.00390625.
 *    - Prevenção de Overflow: O máximo valor teórico possível é sqrt(65535),
 *      que resulta em aproximadamente 255.998. Como a porção inteira máxima é
 *      255 (que cabe perfeitamente em 8 bits) e não há números negativos,
 *      está matematicamente garantido que a operação é IMUNE A OVERFLOW para
 *      qualquer entrada válida de 16 bits.
 *
 * 2. ALGORITMO BIT-A-BIT E CONVERGÊNCIA:
 *    - Base Matemática: O algoritmo computa a raiz através de sucessivas
 *      subtrações e deslocamentos lógicos (método assemelhado ao algoritmo
 *      de extração manual).
 *    - Alinhamento Q16.0 para Q8.8: Ao carregar a entrada Q16.0 nos 16 bits
 *      superiores de uma variável de 32 bits, ela é multiplicada por 2^16,
 *      transformando-se em Q16.16. Como sqrt(x * 2^16) = sqrt(x) * 2^8, o
 *      resultado natural extraído do algoritmo já estará escalonado com os
 *      8 bits inferiores sendo a fração (formato Q8.8).
 *
 * 3. ANÁLISE DE ERRO NUMÉRICO E PRECISÃO:
 *    - Erro de Truncamento: Este método não aproxima o último bit, ele extrai
 *      o piso (floor) do valor exato. Logo, o resultado tem um viés de erro
 *      exclusivamente negativo (sempre menor ou igual à resposta real).
 *    - Erro Máximo Absoluto: O erro da computação será sempre estritamente
 *      menor que a resolução do sistema (Erro < 1 LSB, ou seja, < 0.00390625).
 *
 * 4. CUSTO COMPUTACIONAL (Visando ATmega328):
 *    - Diferente de algoritmos iterativos aproximados (como Newton-Raphson),
 *      o método bit-a-bit possui complexidade de tempo constante O(N), onde
 *      N é o número máximo de deslocamentos.
 *    - Evita completamente a necessidade de operações de divisão em hardware,
 *      sendo otimizado exclusivamente para instruções rápidas da ALU (shift,
 *      subtract, compare), o que o torna a melhor prática para o AVR.
 * ============================================================================
 */

float evaluate_ufix16_q88(const ufix16_t *num);
float evaluate_ufix16_q160(const ufix16_t *num);
ufix16_t take_sqrt(const ufix16_t *input);

int main(void) {
  // assume input Q16.0 e output Q8.8
  ufix16_t num = {0x0, 0xff};
  printf("input is %f\n", evaluate_ufix16_q160(&num));
  ufix16_t res = take_sqrt(&num);
  printf("result is %f\n", evaluate_ufix16_q88(&res));

  return 0;
}

float evaluate_ufix16_q88(const ufix16_t *num) {
  uint16_t raw_value = ((uint16_t)num->MSB << 8) | num->LSB;
  return (float)raw_value / 256.0f;
}

float evaluate_ufix16_q160(const ufix16_t *num) {
  uint16_t raw_value = ((uint16_t)num->MSB << 8) | num->LSB;
  return (float)raw_value;
}

ufix16_t take_sqrt(const ufix16_t *input) {
  // Alinha a entrada de 16 bits (Q16.0) em um espaço de 32 bits.
  // Isso efetivamente multiplica o número por 2^16, convertendo para Q16.16.
  uint32_t val = ((uint32_t)input->MSB << 24) | ((uint32_t)input->LSB << 16);

  uint32_t res = 0; // Acumulador da raiz quadrada (resultado)

  // O 'bit' de teste começa na maior potência de 4 que cabe em 32 bits.
  uint32_t bit = 1UL << 30;

  // Alinha o 'bit' inicial ao tamanho real do valor de entrada
  while (bit > val) {
    bit >>= 2;
  }

  // Laço de extração bit a bit
  while (bit != 0) {
    if (val >= res + bit) {
      val = val - (res + bit);
      res = (res >> 1) + bit;
    } else {
      res >>= 1;
    }
    bit >>= 2;
  }

  // 'res' agora contém a raiz exata no formato Q8.8
  ufix16_t result;
  result.MSB = (res >> 8) & 0xFF;
  result.LSB = res & 0xFF;

  return result;
}
