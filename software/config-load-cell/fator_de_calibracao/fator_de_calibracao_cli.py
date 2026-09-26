import sys
import time
import string
import serial
import serial.tools.list_ports
# from datetime import datetime


def try_parse_int(line: str) -> int | None:
    """Best-effort parsing for the ESP32 calibration stream.

    During calibration the firmware should stream raw integers, but it can also
    print status lines. This helper ignores non-integer lines.
    """
    if line is None:
        return None
    s = line.strip()
    if not s:
        return None
    try:
        return int(float(s))
    except ValueError:
        return None

class Receiver():
    def __init__(
            self,
            port: str,
            baudrate: int = 115200,
            timeout: float = 0.1,
        ):

        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout

        try:
            self.serial:serial.Serial = serial.Serial(
                port=port,
                baudrate=baudrate,
                timeout=timeout,
                xonxoff=False,
                rtscts=False,
                write_timeout=timeout,
                dsrdtr=False,
                inter_byte_timeout=None
            )
        except serial.SerialException as e:
            print(f'WARNING: Serial Exception')
            print(e)
            # self.serial = None
            raise

    def read_response(self):
        return self.serial.readline().decode('utf-8').strip()

    def check_connection(self):
        return self.serial.is_open

    def send_command(self, command):
        self.serial.write(command)

    def close(self):
        self.serial.close()

def get_samples(com: Receiver, n_samples: int = 200) -> list[int]:

    samples = []
    sample_count = 0

    time_between_samples_ms = 1000

    while(com.check_connection() and sample_count < n_samples):

        start_time_ns = time.time_ns()

        raw = com.read_response()
        value = try_parse_int(raw)
        if value is None:
            continue

        samples.append(value)
        sample_count += 1
        print(f"Sample {sample_count}: {value}")

        end_time_ns = time.time_ns()

        elapsed_time_ns = (end_time_ns - start_time_ns)
        elapsed_time_ms = int(elapsed_time_ns * (10**-6))

        await_time_ms = (time_between_samples_ms - elapsed_time_ms)
        if await_time_ms < 0: await_time_ms = 0

        time.sleep(await_time_ms / 1000)

    return samples

def get_calibration_factor(samples: list[int], expected_weight: float):

    samples_sum = sum(samples)
    samples_amnt = len(samples)
    samples_avg = samples_sum/samples_amnt

    calibration_factor = samples_avg / expected_weight

    return calibration_factor

def list_ports():
    if sys.platform.startswith('win'):  # For Windows
        return [port.device for port in serial.tools.list_ports.comports()]
    elif sys.platform.startswith(('linux', 'cygwin')):  # For Linux and Cygwin
        return [port.device for port in serial.tools.list_ports.comports() if port.device.startswith('/dev/ttyUSB') or port.device.startswith('/dev/ttyACM')]
    else:
        print("ERRO: Plataforma não identificada")
        raise

def print_ports() -> None:
    ports = list_ports()
    print("Portas COM disponiveis: ")
    for idx, port in enumerate(ports):
        print(f"({idx}) {port}")

def choose_port_by_index():

    print_ports()

    while True:

        try:
            ports = list_ports() 
            choice = int(input("Digite o número da porta desejada\n-> "))
            port = ports[choice]
            return port

        except ValueError:
            print("ERRO: Valor Inválido!")
            return 'ERROR!!!'

def main(argv: list[str]) -> int:

    # TODO: [x] instantiate comunication interface "com"
    # TODO: [x] make "print_ports()"
    # TODO: [x] make "list_ports()"
    # TODO: [x] make "choose_port_by_index()"

    port = choose_port_by_index()
    com = Receiver(port)

    n_samples = int(input("Digite nº de amostras para calibrar a pesage (recomendado: 200):\n-> "))
    expected_weight = int(input("Digite o peso esperado:\n-> "))

    print(f"\nColoque o peso de {expected_weight} sobre a célula de carga.")
    print("Aguarde...")
    for i in range(5, 0, -1):
        print(f"Iniciando leitura em {i} segundos...", end='\r')
        time.sleep(1)
    print("\nIniciando leitura das amostras...")

    samples = get_samples(com, n_samples=n_samples)

    calibration_factor = get_calibration_factor(samples, float(expected_weight))

    print(f"Fator de Calibração: {calibration_factor}")

    return 0

if __name__ == "__main__":
    exit(main(sys.argv))
