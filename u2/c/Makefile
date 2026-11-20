# ─────────────────────────────────────────────────────────
# Makefile — Sistema de Controle de Nível (ATmega328P)
# ─────────────────────────────────────────────────────────
# Comandos:
#   make          → compila os módulos e gera controle_nivel.hex
#   make flash    → grava o firmware na placa Arduino Uno
#   make size     → exibe consumo de memória Flash e SRAM
#   make clean    → remove os arquivos temporários e binários
# ─────────────────────────────────────────────────────────

MCU        = atmega328p
F_CPU      = 16000000UL
BAUD       = 9600

# Porta e gravador para Arduino Uno (com bootloader Optiboot)
PROGRAMMER = arduino
PORT       ?= $(firstword $(wildcard /dev/ttyUSB* /dev/ttyACM*) /dev/ttyUSB0)
BAUDRATE   = 115200

# Detecção automática do compilador avr-gcc:
# Se avr-gcc não estiver no PATH, procura no toolchain Microchip XC8
AVR_GCC_PATH ?= $(shell which avr-gcc 2>/dev/null)
ifeq ($(AVR_GCC_PATH),)
    XC8_AVR_DIR = /opt/microchip/xc8/v4.00/avr
    ifneq ($(wildcard $(XC8_AVR_DIR)/bin/avr-gcc),)
        CC      = $(XC8_AVR_DIR)/bin/avr-gcc
        OBJCOPY = $(XC8_AVR_DIR)/bin/avr-objcopy
        OBJDUMP = $(XC8_AVR_DIR)/bin/avr-objdump
        SIZE    = $(XC8_AVR_DIR)/bin/avr-size
        DFP_DIR = /opt/microchip/mplabx/v6.35/packs/Microchip/ATmega_DFP/3.6.299
        EXTRA_FLAGS = -B $(DFP_DIR)/gcc/dev/$(MCU) -I $(DFP_DIR)/include
    else
        CC      = avr-gcc
        OBJCOPY = avr-objcopy
        OBJDUMP = avr-objdump
        SIZE    = avr-size
    endif
else
    CC      = avr-gcc
    OBJCOPY = avr-objcopy
    OBJDUMP = avr-objdump
    SIZE    = avr-size
endif

# Detecção automática do avrdude:
# Se não estiver no PATH global, busca no pacote do Arduino CLI / IDE
AVRDUDE_PATH ?= $(shell which avrdude 2>/dev/null)
ifeq ($(AVRDUDE_PATH),)
    ARDUINO_AVRDUDE = $(wildcard $(HOME)/.arduino15/packages/arduino/tools/avrdude/*/bin/avrdude)
    ifneq ($(ARDUINO_AVRDUDE),)
        AVRDUDE = $(firstword $(ARDUINO_AVRDUDE))
        AVRDUDE_CONF = -C $(dir $(AVRDUDE))../etc/avrdude.conf
    else
        AVRDUDE = avrdude
    endif
else
    AVRDUDE = avrdude
endif

SRC        = main.c adc.c temporizador.c usart.c
OBJ        = $(SRC:.c=.o)
TARGET     = controle_nivel

CFLAGS     = -mmcu=$(MCU) \
             $(EXTRA_FLAGS) \
             -DF_CPU=$(F_CPU) \
             -DBAUD=$(BAUD) \
             -Os \
             -Wall -Wextra \
             -std=c11 \
             -funsigned-char \
             -funsigned-bitfields \
             -fpack-struct \
             -fshort-enums

LDFLAGS    = -mmcu=$(MCU) $(EXTRA_FLAGS)

all: $(TARGET).hex size

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

flash: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_CONF) -c $(PROGRAMMER) -p $(MCU) -P $(PORT) -b $(BAUDRATE) -U flash:w:$<:i

size: $(TARGET).elf
	$(SIZE) --mcu=$(MCU) --format=avr $<

clean:
	rm -f $(OBJ) $(TARGET).elf $(TARGET).hex

.PHONY: all flash size clean
