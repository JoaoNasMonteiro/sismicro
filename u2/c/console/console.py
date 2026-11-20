#!/usr/bin/env python3
"""
console.py — Console serial para supervisão e controle de nível de água (ATmega328P)

Funcionalidades:
  - Monitoramento contínuo: nível (barra gráfica e %), estado da bomba e alertas de falha
  - Configuração de parâmetros: limites de ligar (inferior) e desligar (superior)
  - Controle de operação: forçar ligar/desligar bomba e retorno ao modo automático

Protocolo do Pacote (4 bytes binários):
  Byte 0: nivel             (0–255)
  Byte 1: limite_ligar      (0–255)
  Byte 2: limite_desligar   (0–255)
  Byte 3: flags             (Bit 7 = Bomba Ligada, Bit 6 = Falha, Bit 5 = Manual)

Uso:
  python3 console.py                    # Porta padrão /dev/ttyACM0
  python3 console.py --port /dev/ttyUSB0
  python3 console.py --port COM3        # Windows
"""

import argparse
import struct
import sys
import threading
import time

try:
    import serial
except ImportError:
    print("Erro: pyserial não está instalado.")
    print("Instale com: pip install pyserial")
    sys.exit(1)

BAUD_RATE = 9600
TAMANHO_PACOTE = 4


def parse_flags(flags_byte):
    """Decodifica os bits de status enviados pelo microcontrolador."""
    bomba = "LIGADA" if (flags_byte & (1 << 7)) else "DESLIGADA"
    falha = "⚠ FALHA DETECTADA" if (flags_byte & (1 << 6)) else "OK"
    return bomba, falha


def nivel_para_porcentagem(valor):
    """Converte a escala analógica de 0-255 para 0.0-100.0%."""
    return valor * 100.0 / 255.0


def barra_nivel(valor, largura=25):
    """Gera uma representação gráfica em barra do nível de água."""
    preenchido = int((valor / 255.0) * largura)
    return "█" * preenchido + "░" * (largura - preenchido)


def thread_receber(ser, rodando):
    """Thread em segundo plano que recebe e processa os pacotes da USART com auto-sincronização."""
    buffer = bytearray()

    while rodando.is_set():
        try:
            if ser.in_waiting > 0:
                buffer.extend(ser.read(ser.in_waiting))

                while len(buffer) >= TAMANHO_PACOTE:
                    # Auto-sincronismo:
                    # Em um pacote válido:
                    # 1. Os 6 bits inferiores de flags (byte 3) são sempre zero (LF00 0000 -> mask 0x3F == 0)
                    # 2. O limite inferior (byte 1) deve ser menor que o superior (byte 2)
                    if (buffer[3] & 0x3F) != 0 or buffer[1] >= buffer[2]:
                        # Desalinhado por ruído ou abertura de porta: avança 1 byte até sincronizar
                        buffer.pop(0)
                        continue

                    pacote = buffer[:TAMANHO_PACOTE]
                    buffer = buffer[TAMANHO_PACOTE:]

                    nivel = pacote[0]
                    ligar = pacote[1]
                    desligar = pacote[2]
                    flags = pacote[3]

                    bomba, falha = parse_flags(flags)
                    pct = nivel_para_porcentagem(nivel)
                    barra = barra_nivel(nivel)

                    status_cor = "\033[91m" if "FALHA" in falha else "\033[92m"
                    reset_cor = "\033[0m"

                    print(f"\r{'═' * 55}")
                    print(f"  Nível do Tanque: [{barra}] {pct:5.1f}% ({nivel}/255)")
                    print(f"  Bomba: {bomba:<9} | Status: {status_cor}{falha}{reset_cor}")
                    print(f"  Limites: Liga <= {nivel_para_porcentagem(ligar):.1f}% ({ligar}) | Desliga >= {nivel_para_porcentagem(desligar):.1f}% ({desligar})")
                    print(f"{'═' * 55}")
                    print("> ", end="", flush=True)
            else:
                time.sleep(0.05)
        except serial.SerialException:
            break


def enviar_pacote(ser, nivel, ligar, desligar, flags):
    """Envia o pacote estruturado de 4 bytes pela serial."""
    pacote = struct.pack("BBBB", nivel, ligar, desligar, flags)
    ser.write(pacote)


def mostrar_ajuda():
    print("""
╔══════════════════════════════════════════════════════════╗
║               COMANDOS DO OPERADOR                       ║
╠══════════════════════════════════════════════════════════╣
║  ligar <0-255>     Ajusta o limite inferior (ex: 51)     ║
║  desligar <0-255>  Ajusta o limite superior (ex: 204)    ║
║  ajuda             Exibe este menu                       ║
║  sair              Fecha o console serial                ║
╚══════════════════════════════════════════════════════════╝
""")


def main():
    import glob
    portas = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    porta_padrao = portas[0] if portas else "/dev/ttyUSB0"

    parser = argparse.ArgumentParser(description="Console Serial de Controle de Nível")
    parser.add_argument("--port", default=porta_padrao, help=f"Porta serial (padrão: {porta_padrao})")
    parser.add_argument("--baud", type=int, default=BAUD_RATE, help="Baud rate (padrão: 9600)")
    args = parser.parse_args()

    print(f"Tentando conectar em {args.port} a {args.baud} baud...")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
        time.sleep(1.0)  # Aguarda o bootloader do Arduino estabilizar após DTR reset
        ser.reset_input_buffer()
        ser.reset_output_buffer()
    except serial.SerialException as e:
        print(f"Erro ao abrir porta serial {args.port}: {e}")
        print("Dica: Verifique a conexão USB e permissões ('sudo chmod a+rw /dev/ttyACM0').")
        sys.exit(1)

    print("Conexão estabelecida com sucesso! Digite 'ajuda' para comandos.\n")

    limite_ligar_atual = 51
    limite_desligar_atual = 204

    rodando = threading.Event()
    rodando.set()
    t = threading.Thread(target=thread_receber, args=(ser, rodando), daemon=True)
    t.start()

    try:
        while True:
            try:
                cmd = input("> ").strip().lower()
            except EOFError:
                break

            if not cmd:
                continue

            if cmd == "sair":
                break

            elif cmd == "ajuda":
                mostrar_ajuda()

            elif cmd.startswith("ligar "):
                try:
                    valor = int(cmd.split()[1])
                    if 0 <= valor < limite_desligar_atual:
                        limite_ligar_atual = valor
                        enviar_pacote(ser, 0, limite_ligar_atual, limite_desligar_atual, 0)
                        print(f"  → Limite inferior alterado para {valor} ({nivel_para_porcentagem(valor):.1f}%)")
                    else:
                        print(f"  Erro: limite_ligar deve ser menor que limite_desligar ({limite_desligar_atual})")
                except (ValueError, IndexError):
                    print("  Uso correto: ligar <0-255>")

            elif cmd.startswith("desligar "):
                try:
                    valor = int(cmd.split()[1])
                    if limite_ligar_atual < valor <= 255:
                        limite_desligar_atual = valor
                        enviar_pacote(ser, 0, limite_ligar_atual, limite_desligar_atual, 0)
                        print(f"  → Limite superior alterado para {valor} ({nivel_para_porcentagem(valor):.1f}%)")
                    else:
                        print(f"  Erro: limite_desligar deve ser maior que limite_ligar ({limite_ligar_atual})")
                except (ValueError, IndexError):
                    print("  Uso correto: desligar <0-255>")

            else:
                print(f"  Comando desconhecido: '{cmd}'. Digite 'ajuda'.")

    except KeyboardInterrupt:
        print("\nEncerrando sessão...")

    rodando.clear()
    t.join(timeout=1)
    ser.close()
    print("Conexão encerrada.")


if __name__ == "__main__":
    main()
