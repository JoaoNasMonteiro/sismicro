#include "temporizador.h"
#include "adc.h"

#include <avr/interrupt.h>
#include <avr/io.h>

/* Flag que indica pro main que o timer disparou */
volatile uint8_t flag_tempo = 0;

void timer_init(void) {
    /* Zera o contador do timer */
    TCNT1 = 0;

    /*
     * Valor até onde o timer conta antes de resetar.
     * 15624 = 1 segundo com prescaler 1024 e clock de 16 MHz.
     *
     * Se quiser mudar o tempo, é só trocar esse número:
     *   - 7812  = 0.5 segundo (500 ms)
     *   - 3906  = 0.25 segundo (250 ms)
     *   - 15624 = 1 segundo
     *   - 31249 = 2 segundos
     */
    OCR1A = 15624;

    /*
     * TCCR1A = 0 → não usa PWM, modo normal de saída
     *
     * TCCR1B:
     *   WGM12 = 1 → ativa modo CTC (Clear Timer on Compare Match)
     *   CS12  = 1, CS10 = 1 → prescaler de 1024
     */
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);

    /*
     * TIMSK1:
     *   OCIE1A = 1 → habilita a interrupção quando TCNT1 == OCR1A
     */
    TIMSK1 = (1 << OCIE1A);
}

/*
 * Essa função roda SOZINHA toda vez que o timer chega em OCR1A.
 * Você não chama ela no código — o hardware chama automaticamente.
 */
ISR(TIMER1_COMPA_vect) {
    flag_tempo = 1;

    /* Manda o ADC começar uma nova leitura do sensor */
    adc_iniciar_conversao();
}
