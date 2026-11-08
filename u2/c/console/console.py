#!/usr/bin/env python3
"""
Console USART para comunicação entre o computador e o ATmega328P.
Recebe, traduz, exibe, altera parâmetros e envia comandos no formato `pacote_t`.
"""

import argparse
from dataclasses import dataclass
import os
import select
import struct
import sys
import threading
import time
from typing import Optional

try:
    import serial  # type: ignore
except ImportError:
    serial = None

try:
    import termios
    import tty
except ImportError:
    termios = None  # type: ignore
    tty = None  # type: ignore

# Constantes alinhadas com `usart.h`
CAPACIDADE_MAXIMA = 255
NIVEL_MAX_SEGURANCA = 245
NIVEL_LIGAR_PADRAO = 64
AMOSTRAGEM_MODO_MAX = 3

FLAG_LIGADA = 1 << 7  # Bit 7 (L): 1 = bomba ligada, 0 = desligada
FLAG_FALHA = 1 << 6   # Bit 6 (F): 1 = falha detectada, 0 = operação normal
FLAG_FORCAR = 1 << 5  # Bit 5: 1 = comando para forçar ligar/desligar

FORMATO_PACOTE = struct.Struct("<BBBB")

DESCRICAO_AMOSTRAGEM = {
    0: "Modo 0 (Lenta)",
    1: "Modo 1 (Normal)",
    2: "Modo 2 (Rápida)",
    3: "Modo 3 ( Contínua / Muito Rápida)",
}


@dataclass
class Pacote:
    """Representação em Python da struct `pacote_t` (4 bytes)."""

    nivel: int = 0
    ligar: int = NIVEL_LIGAR_PADRAO
    amostragem: int = 1
    flags: int = 0

    def empacotar(self) -> bytes:
        """Serializa o pacote em 4 bytes (uint8_t x 4)."""
        return FORMATO_PACOTE.pack(
            self.nivel & 0xFF,
            self.ligar & 0xFF,
            self.amostragem & 0xFF,
            self.flags & 0xFF,
        )

    @classmethod
    def desempacotar(cls, dados: bytes) -> "Pacote":
        """Reconstrói um `Pacote` a partir de 4 bytes recebidos via USART."""
        if len(dados) != FORMATO_PACOTE.size:
            raise ValueError(f"Esperados {FORMATO_PACOTE.size} bytes, recebidos {len(dados)}.")
        nivel, ligar, amostragem, flags = FORMATO_PACOTE.unpack(dados)
        return cls(nivel=nivel, ligar=ligar, amostragem=amostragem, flags=flags)

    @property
    def bomba_ligada(self) -> bool:
        return bool(self.flags & FLAG_LIGADA)

    @property
    def falha_detectada(self) -> bool:
        return bool(self.flags & FLAG_FALHA)

    def traduzir(self) -> str:
        """Traduz os bytes do pacote para uma visualização legível para o operador."""
        perc_nivel = (self.nivel / CAPACIDADE_MAXIMA) * 100.0
        perc_ligar = (self.ligar / CAPACIDADE_MAXIMA) * 100.0
        largura_barra = 25
        preenchido = int(round((self.nivel / CAPACIDADE_MAXIMA) * largura_barra))
        barra = "#" * preenchido + "-" * (largura_barra - preenchido)

        estado_bomba = "LIGADA (ON)" if self.bomba_ligada else "DESLIGADA (OFF)"
        estado_falha = (
            "ALERTA: FALHA DETECTADA (bomba ligada sem subida de nível!)"
            if self.falha_detectada
            else "NORMAL (Sem falhas)"
        )
        desc_amostragem = DESCRICAO_AMOSTRAGEM.get(
            self.amostragem, f"Modo {self.amostragem}"
        )

        linhas = [
            "=" * 64,
            "                 ESTADO DO RESERVATÓRIO (USART)                 ",
            "=" * 64,
            f"  Nível de Água     : {self.nivel:3d} / {CAPACIDADE_MAXIMA} L ({perc_nivel:5.1f}%) [{barra}]",
            f"  Limiar p/ Ligar   : {self.ligar:3d} / {CAPACIDADE_MAXIMA} L ({perc_ligar:5.1f}%)",
            f"  Taxa de Amostragem: {desc_amostragem}",
            f"  Estado da Bomba   : {estado_bomba}",
            f"  Diagnóstico       : {estado_falha}",
            f"  Byte de Flags     : 0b{self.flags:08b} (L={int(self.bomba_ligada)}, F={int(self.falha_detectada)})",
            "=" * 64,
        ]
        return "\n".join(linhas)


class PosixSerial:
    """Fallback de porta serial usando `termios` caso `pyserial` não esteja instalado."""

    BAUD_MAP = {
        4800: getattr(termios, "B4800", None),
        9600: getattr(termios, "B9600", None),
        19200: getattr(termios, "B19200", None),
        38400: getattr(termios, "B38400", None),
        57600: getattr(termios, "B57600", None),
        115200: getattr(termios, "B115200", None),
    }

    def __init__(self, port: str, baudrate: int = 9600, timeout: float = 0.2):
        if termios is None:
            raise RuntimeError("Nem `pyserial` nem `termios` estão disponíveis neste sistema.")
        if baudrate not in self.BAUD_MAP or self.BAUD_MAP[baudrate] is None:
            raise ValueError(f"Baud rate não suportado pelo fallback POSIX: {baudrate}")

        self.port = port
        self.timeout = timeout
        self.fd: Optional[int] = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)

        attrs = termios.tcgetattr(self.fd)
        baud_const = self.BAUD_MAP[baudrate]
        # 8N1, sem controle de fluxo, modo raw
        attrs[0] = 0  # iflag
        attrs[1] = 0  # oflag
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL  # cflag
        attrs[3] = 0  # lflag
        attrs[4] = baud_const  # ispeed
        attrs[5] = baud_const  # ospeed
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        termios.tcflush(self.fd, termios.TCIOFLUSH)

    def read(self, size: int = 1) -> bytes:
        if self.fd is None:
            return b""
        buf = bytearray()
        deadline = time.monotonic() + self.timeout
        while len(buf) < size:
            restante = deadline - time.monotonic()
            if restante <= 0:
                break
            prontos, _, _ = select.select([self.fd], [], [], restante)
            if not prontos:
                break
            chunk = os.read(self.fd, size - len(buf))
            if not chunk:
                break
            buf.extend(chunk)
        return bytes(buf)

    def write(self, data: bytes) -> int:
        if self.fd is None:
            return 0
        return os.write(self.fd, data)

    def close(self) -> None:
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None


class SimuladorPlaca:
    """Simulador local do ATmega328P para testes do console sem hardware conectado."""

    def __init__(self):
        self.estado = Pacote(
            nivel=100,
            ligar=NIVEL_LIGAR_PADRAO,
            amostragem=1,
            flags=0,
        )
        self._rx_buffer = bytearray()
        self._ultimo_envio = 0.0
        self._lock = threading.Lock()

    def read(self, size: int = 4) -> bytes:
        intervalos = {0: 2.0, 1: 1.0, 2: 0.5, 3: 0.25}
        intervalo = intervalos.get(self.estado.amostragem, 1.0)
        agora = time.monotonic()
        if agora - self._ultimo_envio < intervalo:
            time.sleep(min(0.1, intervalo - (agora - self._ultimo_envio)))
            return b""

        with self._lock:
            self._ultimo_envio = time.monotonic()
            nivel_anterior = self.estado.nivel

            # Dinâmica simples do reservatório
            if self.estado.bomba_ligada:
                self.estado.nivel = min(CAPACIDADE_MAXIMA, self.estado.nivel + 5)
            else:
                self.estado.nivel = max(0, self.estado.nivel - 3)

            # Controle automático por limiares
            if self.estado.nivel <= self.estado.ligar and not self.estado.falha_detectada:
                self.estado.flags |= FLAG_LIGADA
            elif self.estado.nivel >= NIVEL_MAX_SEGURANCA:
                self.estado.flags &= ~FLAG_LIGADA

            # Detecção de falha (bomba ligada mas nível não subiu)
            if self.estado.bomba_ligada and self.estado.nivel <= nivel_anterior:
                self.estado.flags |= FLAG_FALHA

            dados = self.estado.empacotar()
            return dados[:size]

    def write(self, data: bytes) -> int:
        with self._lock:
            self._rx_buffer.extend(data)
            while len(self._rx_buffer) >= FORMATO_PACOTE.size:
                raw = bytes(self._rx_buffer[: FORMATO_PACOTE.size])
                del self._rx_buffer[: FORMATO_PACOTE.size]
                cmd = Pacote.desempacotar(raw)

                self.estado.ligar = min(cmd.ligar, NIVEL_MAX_SEGURANCA)
                if cmd.amostragem <= AMOSTRAGEM_MODO_MAX:
                    self.estado.amostragem = cmd.amostragem

                if cmd.flags & FLAG_FORCAR:
                    if cmd.flags & FLAG_LIGADA:
                        if not self.estado.falha_detectada and self.estado.nivel < NIVEL_MAX_SEGURANCA:
                            self.estado.flags |= FLAG_LIGADA
                    else:
                        if self.estado.nivel >= self.estado.ligar:
                            self.estado.flags &= ~FLAG_LIGADA
        return len(data)

    def close(self) -> None:
        pass


class ConsoleUSART:
    """Gerencia a recepção contínua, tradução e envio de pacotes pela USART."""

    def __init__(self, porta_serial):
        self.serial = porta_serial
        self.ultimo_pacote = Pacote()
        self.pacotes_recebidos = 0
        self.monitorar_ao_vivo = False
        self.executando = True
        self._lock = threading.Lock()
        self._thread_rx = threading.Thread(target=self._loop_recepcao, daemon=True)

    def iniciar(self) -> None:
        self._thread_rx.start()

    def encerrar(self) -> None:
        self.executando = False
        if self._thread_rx.is_alive():
            self._thread_rx.join(timeout=1.0)
        self.serial.close()

    def _loop_recepcao(self) -> None:
        buffer = bytearray()
        while self.executando:
            try:
                chunk = self.serial.read(FORMATO_PACOTE.size - len(buffer))
            except Exception as exc:
                print(f"\n[ERRO USART RX]: {exc}")
                time.sleep(0.5)
                continue

            if not chunk:
                continue

            buffer.extend(chunk)
            if len(buffer) >= FORMATO_PACOTE.size:
                raw = bytes(buffer[: FORMATO_PACOTE.size])
                del buffer[: FORMATO_PACOTE.size]
                pkt = Pacote.desempacotar(raw)

                with self._lock:
                    falha_nova = pkt.falha_detectada and not self.ultimo_pacote.falha_detectada
                    self.ultimo_pacote = pkt
                    self.pacotes_recebidos += 1

                if falha_nova:
                    print("\n[!] ALERTA CRÍTICO: Falha detectada no abastecimento da bomba!")

                if self.monitorar_ao_vivo:
                    print(f"\n{pkt.traduzir()}")

    def obter_estado(self) -> Pacote:
        with self._lock:
            return Pacote(
                nivel=self.ultimo_pacote.nivel,
                ligar=self.ultimo_pacote.ligar,
                amostragem=self.ultimo_pacote.amostragem,
                flags=self.ultimo_pacote.flags,
            )

    def enviar_pacote(self, pkt: Pacote) -> None:
        raw = pkt.empacotar()
        self.serial.write(raw)
        bytes_hex = " ".join(f"0x{b:02X}" for b in raw)
        print(f"\n[TX -> ATmega328P] Enviado ({len(raw)} bytes: {bytes_hex})")

    def alterar_limiar_ligar(self, novo_ligar: int) -> None:
        if not (0 <= novo_ligar <= NIVEL_MAX_SEGURANCA):
            print(f"[!] Valor inválido. Escolha entre 0 e {NIVEL_MAX_SEGURANCA} L.")
            return
        with self._lock:
            self.ultimo_pacote.ligar = novo_ligar
            cmd = Pacote(
                nivel=self.ultimo_pacote.nivel,
                ligar=self.ultimo_pacote.ligar,
                amostragem=self.ultimo_pacote.amostragem,
                flags=self.ultimo_pacote.flags & (FLAG_LIGADA | FLAG_FALHA),
            )
        self.enviar_pacote(cmd)
        print(
            f"[OK] Limiar de acionamento atualizado para {novo_ligar} L "
            f"({(novo_ligar / CAPACIDADE_MAXIMA) * 100:.1f}%)."
        )

    def alterar_amostragem(self, novo_modo: int) -> None:
        if not (0 <= novo_modo <= AMOSTRAGEM_MODO_MAX):
            print(f"[!] Modo inválido. Escolha entre 0 e {AMOSTRAGEM_MODO_MAX}.")
            return
        with self._lock:
            self.ultimo_pacote.amostragem = novo_modo
            cmd = Pacote(
                nivel=self.ultimo_pacote.nivel,
                ligar=self.ultimo_pacote.ligar,
                amostragem=self.ultimo_pacote.amostragem,
                flags=self.ultimo_pacote.flags & (FLAG_LIGADA | FLAG_FALHA),
            )
        self.enviar_pacote(cmd)
        print(f"[OK] Taxa de amostragem atualizada para: {DESCRICAO_AMOSTRAGEM[novo_modo]}.")

    def forcar_bomba(self, ligar: bool) -> None:
        atual = self.obter_estado()

        # Verificação local das ressalvas de segurança antes de enviar (a placa também valida)
        if ligar:
            if atual.falha_detectada:
                print(
                    "[RESSALVA] A bomba está com falha sinalizada! "
                    "O comando será enviado, mas o ATmega328P bloqueará por segurança."
                )
            elif atual.nivel >= NIVEL_MAX_SEGURANCA:
                print(
                    f"[RESSALVA] Reservatório já está cheio ({atual.nivel} L >= {NIVEL_MAX_SEGURANCA} L). "
                    "Acionamento bloqueado para evitar transbordamento."
                )
            flags = (atual.flags & FLAG_FALHA) | FLAG_FORCAR | FLAG_LIGADA
        else:
            if atual.nivel < atual.ligar:
                print(
                    f"[RESSALVA] Nível atual ({atual.nivel} L) está abaixo do mínimo configurado "
                    f"({atual.ligar} L). O desligamento forçado pode ser recusado pela placa."
                )
            flags = (atual.flags & FLAG_FALHA) | FLAG_FORCAR

        cmd = Pacote(
            nivel=atual.nivel,
            ligar=atual.ligar,
            amostragem=atual.amostragem,
            flags=flags,
        )
        self.enviar_pacote(cmd)
        acao = "LIGAR" if ligar else "DESLIGAR"
        print(f"[OK] Comando para forçar {acao} enviado ao ATmega328P.")


def abrir_conexao(porta: str, baud: int, simular: bool):
    if simular:
        print("[INFO] Executando em modo de SIMULAÇÃO local (--sim).")
        return SimuladorPlaca()

    if serial is not None:
        return serial.Serial(port=porta, baudrate=baud, timeout=0.2)

    print("[INFO] `pyserial` não encontrado; utilizando driver POSIX `termios` nativo.")
    return PosixSerial(port=porta, baudrate=baud, timeout=0.2)


def menu_interativo(console: ConsoleUSART) -> None:
    opcoes = """
----------------------------------------------------------------
Menu de Operação (Computador <-> ATmega328P):
  [1] Visualizar estado atual do reservatório e parâmetros
  [2] Alterar limiar de acionamento da bomba (% ou Litros)
  [3] Alterar modo da taxa de amostragem (0 a 3)
  [4] Forçar LIGAR a bomba (com ressalva)
  [5] Forçar DESLIGAR a bomba (com ressalva)
  [6] Alternar monitoramento automático em tempo real (ON/OFF)
  [0] Sair
----------------------------------------------------------------"""

    while True:
        print(opcoes)
        try:
            escolha = input("Escolha uma opção: ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nEncerrando console...")
            break

        if escolha == "1":
            pkt = console.obter_estado()
            print(f"\nPacotes recebidos até o momento: {console.pacotes_recebidos}")
            print(pkt.traduzir())

        elif escolha == "2":
            modo = input("Deseja informar em [P]orcentagem (0-96%) ou [L]itros (0-245)? [P/L]: ").strip().upper()
            try:
                if modo == "P":
                    perc = float(input("Informe o percentual para ligar a bomba (0 a 96): ").strip())
                    valor_ligar = int(round((perc / 100.0) * CAPACIDADE_MAXIMA))
                else:
                    valor_ligar = int(input(f"Informe o nível em litros (0 a {NIVEL_MAX_SEGURANCA}): ").strip())
                console.alterar_limiar_ligar(valor_ligar)
            except ValueError:
                print("[!] Entrada numérica inválida.")

        elif escolha == "3":
            print("Modos disponíveis:")
            for k, desc in DESCRICAO_AMOSTRAGEM.items():
                print(f"  {k}: {desc}")
            try:
                novo_modo = int(input(f"Escolha o modo (0 a {AMOSTRAGEM_MODO_MAX}): ").strip())
                console.alterar_amostragem(novo_modo)
            except ValueError:
                print("[!] Entrada numérica inválida.")

        elif escolha == "4":
            console.forcar_bomba(ligar=True)

        elif escolha == "5":
            console.forcar_bomba(ligar=False)

        elif escolha == "6":
            console.monitorar_ao_vivo = not console.monitorar_ao_vivo
            estado_mon = "ATIVADO" if console.monitorar_ao_vivo else "DESATIVADO"
            print(f"[INFO] Monitoramento em tempo real {estado_mon}.")

        elif escolha == "0":
            print("Encerrando console...")
            break

        else:
            print("[!] Opção inválida.")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Console USART para supervisão e controle do reservatório no ATmega328P."
    )
    parser.add_argument(
        "-p",
        "--port",
        default="/dev/ttyACM0",
        help="Porta serial conectada ao ATmega328P (padrão: /dev/ttyACM0)",
    )
    parser.add_argument(
        "-b",
        "--baud",
        type=int,
        default=9600,
        help="Taxa de transmissão em bps (padrão: 9600)",
    )
    parser.add_argument(
        "--sim",
        action="store_true",
        help="Executa com simulador local do ATmega328P sem exigir hardware conectado",
    )
    args = parser.parse_args()

    try:
        conexao = abrir_conexao(args.port, args.baud, args.sim)
    except Exception as exc:
        print(f"[ERRO] Não foi possível abrir a porta serial '{args.port}': {exc}", file=sys.stderr)
        print("Dica: verifique a porta (-p /dev/ttyUSB0) ou use --sim para simular a placa.", file=sys.stderr)
        return 1

    console = ConsoleUSART(conexao)
    console.iniciar()
    try:
        menu_interativo(console)
    finally:
        console.encerrar()
    return 0


if __name__ == "__main__":
    sys.exit(main())
