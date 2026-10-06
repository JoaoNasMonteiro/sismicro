#include <stdint.h>
#include <stdio.h>

typedef uint8_t REG_t;

typedef struct ufix16 {
  REG_t MSB;
  REG_t LSB;
} ufix16_t;

// entrada Q16.0 saída Q8.8

float evaluate_ufix16_q88(const ufix16_t *num);
float evaluate_ufix16_q160(const ufix16_t *num);
ufix16_t take_sqrt(const ufix16_t *input);

int main(void) {
  // treat input as Q16.0 to make it so that the result is in Q8.8
  ufix16_t num = {0x0, 0xff};
  printf("input is %f\n", evaluate_ufix16_q160(&num));
  ufix16_t res = take_sqrt(&num);
  printf("result is %f\n", evaluate_ufix16_q88(&res));

  return 0;
}

// converts Q8.8 to float
float evaluate_ufix16_q88(const ufix16_t *num) {
  uint16_t raw_value = ((uint16_t)num->MSB << 8) | num->LSB;
  return (float)raw_value / 256.0f;
}

float evaluate_ufix16_q160(const ufix16_t *num) {
  uint16_t raw_value = ((uint16_t)num->MSB << 8) | num->LSB;
  return (float)raw_value;
}

// takes the sqrt of an ufix16_t assumed to be in Q16.0 and returns the result
// in Q8.8
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
