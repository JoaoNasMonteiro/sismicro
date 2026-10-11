.ORG 0x00
    
; Incializa o SP
LDI R16, HIGH(RAMEND)
OUT SPH, R16
LDI R16, LOW(RAMEND)
OUT SPL, R16
JMP MAIN
    
; Carregando o registrador T
CALCULA_T: 
MOV R21, R17
LSL R21
LSL R21
LDI R29, 0x01
ADD R21, R29
MOV R22, R18
SUB R22, R21
MOV R19, R22
BRMI NEGATIVO
LSL R17
MOV R18, R19
SBR R17, 0x01
RET
    
    
NEGATIVO:
LSL R17
RET

    
; Carregando o registrador A
SHIFT_DOIS:
LDI R20, 0x02 ; contador para o shift duplo
SHIFT:
LSL R16
CALL CARRY
DEC R20
BRNE SHIFT
RET 

CARRY:
BRCS CS
LSL R18
CBR R18, 0x01
CLC
RET 
CS: 
LSL R18
SBR R18, 0x01
CLC	
RET 

MAIN: ; Supõe-se que os números já estão todos em Q4.4
LDI R16, 0x01 ; carrega o registrador com o valor para calcular a raiz (X)
LDI R17, 0x00 ; carrega o registrador da resposta (Q)
LDI R18, 0x00 ; carrega o registrador com o valor atual sendo trabalhado (A)
LDI R19, 0x00 ; carrega o registrador de sinal (T)
LDI R30, 0x06 ; carrega o registrador de contagem da quantidade de cálculos
    
LOOP:
CALL SHIFT_DOIS
CALL CALCULA_T
DEC R30
BRNE LOOP
    

FINAL: RJMP FINAL


