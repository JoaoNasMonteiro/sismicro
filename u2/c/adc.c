#include "adc.h"

#include <avr/interrupt.h>
#include <avr/io.h>

/* Variáveis que a interrupção do ADC vai preencher */
volatile uint8_t flag_leitura_pronta = 0;
volatile uint8_t nivel_atual         = 0;

void adc_init(void) {
    /*
     * ADMUX — escolhe a referência de tensão e o canal
     *
     * REFS0 = 1  → usa AVCC (5V) como referência
     * ADLAR = 1  → alinha o resultado pra esquerda
     *              (assim o ADCH já tem os 8 bits que a gente quer)
     * MUX   = 0  → lê o canal ADC0, que é o pino A0 (PC0)
     */
    ADMUX = (1 << REFS0) | (1 << ADLAR);

    /*
     * ADCSRA — controle principal do ADC
     *
     * ADEN  = 1  → liga o ADC
     * ADIE  = 1  → habilita a interrupção de "conversão pronta"
     * ADPS2 = 1, ADPS1 = 1, ADPS0 = 1 → prescaler de 128
     */
    ADCSRA = (1 << ADEN)
           | (1 << ADIE)
           | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

void adc_iniciar_conversao(void) {
    /* Seta o bit ADSC = 1 pra mandar o ADC começar a converter */
    ADCSRA |= (1 << ADSC);
}

ISR(ADC_vect) {
    /* Lê o valor convertido (0-255) */
    nivel_atual = ADCH;

    /* Avisa o main que a leitura tá pronta */
    flag_leitura_pronta = 1;
}
