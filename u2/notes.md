# Notas de reunião
bomba   | a. <- Controlador <- ATMEGA <-(USART)- Computador
        | b. -> ADC -> ATMEGA -(usart)-> Computador

Parâmetros:
    - % de ligar/desligar
    - Taxa de amostragem
b.
O sensor envia nível da água a cada x ciclos e os parâmetros.
Capacidade fixa em 255L
O ATMEGA envia este valor para o computador
Além disso deve enviar informações sobre o estado operacional (ligada/desligada) e sobre possíveis falhas:
    - Ligada/desligada e falha/não falha em um bit separado
    - Identificação de falha:
        - Se a bomba estiver ligada e o reservatório estiver diminuindo ou não
O computador recebe as informações e mostra na tela

a.
O computador envia para o ATMEGA a qualquer momento:
    - Mudanças nos parâmetros
    - Forçar ligar/desligar (com ressalva)

O ATMEGA deve implementar dos parâmetros

Formato dos dados:
```C
struct dados {
    uint8_t nível;
    uint8_t ligar; // fração de 255 para a qual a bomba liga
    uint8_t amostragem; //taxa de amostragem. Modos fixos 0 - y
    uint8_t Flags //top 2 bits para flag, LF00 0000
};
```


