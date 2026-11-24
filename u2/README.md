# Sistema de Controle de Nível de Água

Implementação embarcada para um reservatório controlado por bomba elétrica, desenvolvida em linguagem C para o microcontrolador **ATmega328P**. O nível do reservatório é medido pelo ADC e a bomba é acionada segundo limites configuráveis.

O projeto também possui um console em Python para monitorar o nível e enviar novos limites pela porta serial.

## Funcionamento

O nível do sensor analógico conectado ao pino **A0 (PC0)** é convertido pelo ADC em um valor inteiro de 0 a 255:

- `0`: reservatório vazio;
- `255`: reservatório cheio.

O Timer1 gera um evento a cada 1 segundo. Nesse evento, o ADC inicia uma conversão e a interrupção do ADC salva o valor no programa principal. A bomba é conectada ao pino **PB0**.

Os limites padrão são:

| Parâmetro | Valor | Nível aproximado |
|---|---:|---:|
| Liga a bomba | 51 | <= 20% |
| Desliga a bomba | 204 | >= 80% |

Assim, a bomba liga quando o nível fica menor ou igual ao limite inferior e desliga quando atinge ou ultrapassa o limite superior. A distância entre os limites evita acionamentos repetitivos.

Quando a bomba está ligada, o sistema compara o nível atual com o nível da leitura anterior. Se o nível não aumentar em cinco leituras consecutivas, o firmware ativa o indicador de falha.

## Comunicação serial

A comunicação entre o ATmega328P e o computador utiliza USART0 a 9600 bps. Os dados enviados pelo microcontrolador são organizados em pacotes de 4 bytes:

| Posição | Conteúdo |
|---|---|
| 0 | Nível, de 0 a 255 |
| 1 | Limite inferior de acionamento |
| 2 | Limite superior de acionamento |
| 3 | Flags de estado |

Os bits do quarto byte possuem os seguintes significados:

- **Bit 7:** bomba ligada;
- **Bit 6:** falha detectada;
- **Bits 5 a 0:** não utilizados.

O console pode enviar um novo limite inferior e um novo limite superior. Ao receber um pacote de 4 bytes, o firmware atualiza os dois limites antes da próxima leitura.

## Estrutura dos arquivos

```text
u2/
|-- c/
|   |-- main.c              Lógica principal e controle da bomba
|   |-- adc.c / adc.h       Configuração e leitura do ADC
|   |-- temporizador.c/h    Base temporal do Timer1
|   |-- usart.c / usart.h   Recepção e envio pela USART
|   |-- console/console.py  Interface serial em Python
|   `-- Makefile            Compilação e gravação do firmware
|-- Problema_2.pdf          Enunciado da atividade
|-- relatorio/              Relatório técnico
`-- README.md
```