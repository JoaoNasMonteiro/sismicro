#ifndef ADC_H
#define ADC_H

#include <stdint.h>

/*
 * Essa flag vira 1 quando o ADC termina de ler o sensor.
 * O main.c checa essa flag pra saber que tem um valor novo pronto.
 */
extern volatile uint8_t flag_leitura_pronta;

/*
 * Valor lido do sensor, de 0 a 255.
 * 0 = reservatório vazio, 255 = reservatório cheio.
 */
extern volatile uint8_t nivel_atual;

/* Configura o ADC pra ler o pino A0 (PC0) */
void adc_init(void);

/* Manda o ADC começar uma conversão (chamada pelo timer) */
void adc_iniciar_conversao(void);

#endif
