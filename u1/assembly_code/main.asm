.ORG 0x00
; Entrada (R19:18)
.DEF INPUT_HIGH = R19
.DEF INPUT_LOW = R18
    
; Resposta (R21:R20)
.DEF RESP_HIGH = R21
.DEF RESP_LOW = R20
    
;Valor sendo trabalhado (R22:R23)
.DEF A_HIGH = R23
.DEF A_LOW = R22
 
; Registrador do sinal (R25:R24)
.DEF SINAL_HIGH = R25
.DEF SINAL_LOW = R24

; Registrador da resposta temporária (R27:R26)
.DEF RESP_TEMP_HIGH = R27
.DEF RESP_TEMP_LOW = R26
   
; Incializa o SP
LDI R16, HIGH(RAMEND)
OUT SPH, R16
LDI R16, LOW(RAMEND)
OUT SPL, R16
JMP MAIN
    
; Carregando o registrador de sinal (T)
CALCULA_T: 
MOVW RESP_TEMP_HIGH:RESP_TEMP_LOW, RESP_HIGH:RESP_LOW
LDI R28, 0x02 ; contador para o shift duplo para montar {RESP,01}
SHIFT_SINAL:
LSL RESP_TEMP_LOW
ROL RESP_TEMP_HIGH
DEC R28
BRNE SHIFT_SINAL
SBR RESP_TEMP_LOW, 0x01
MOVW SINAL_HIGH:SINAL_LOW, A_HIGH:A_LOW
SUB SINAL_LOW, RESP_TEMP_LOW
SBC SINAL_HIGH, RESP_TEMP_HIGH
    
BRMI NEGATIVO

LSL RESP_LOW
ROL RESP_HIGH
MOVW A_HIGH:A_LOW, SINAL_HIGH:SINAL_LOW
SBR RESP_LOW, 0x01
RET    
    
NEGATIVO:
LSL RESP_LOW
ROL RESP_HIGH
RET
    
; Carregando o registrador com o valor que será usado (A)
SHIFT_DOIS:
LDI R29, 0x02 ; contador para o shift duplo
SHIFT:
LSL INPUT_LOW
ROL INPUT_HIGH
ROL A_LOW
ROL A_HIGH
DEC R29
BRNE SHIFT
RET 

 
MAIN: ; Supõe-se entrada em Q16.0 e saída em Q8.8
LDI INPUT_HIGH, 0xFF ; carrega o registrador com o valor HIGH para calcular a raiz (X)
LDI INPUT_LOW, 0xFF ; carrega o registrador com o valor LOW para calcular a raiz (X)

LDI RESP_HIGH, 0x00 ; carrega o registrador HIGH da resposta (Q)
LDI RESP_LOW, 0x00 ; carrega o registrador LOW da resposta (Q)
    
LDI A_HIGH, 0x00 ; carrega o registrador com o valor HIGH atual sendo trabalhado (A)
LDI A_LOW, 0x00 ; carrega o registrador com o valor LOW atual sendo trabalhado (A)

LDI SINAL_HIGH, 0x00 ; carrega o registrador HIGH de sinal (T)
LDI SINAL_LOW, 0x00 ; carrega o registrador LOW de sinal (T)

LDI R30, 0x10 ; carrega o contador da quantidade de cálculos
    
LOOP:
CALL SHIFT_DOIS
CALL CALCULA_T
DEC R30
BRNE LOOP
    
FINAL: RJMP FINAL


