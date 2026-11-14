import sys
from PySide6.QtWidgets import QApplication, QMainWindow, QMessageBox
from PySide6.QtCore import QThread, Signal
import fator_de_calibracao_cli as fact_cli
import ui_fator_de_calibracao as fact_ui
import logging


class ConfigModeThread(QThread):
    """Manda INIT CONFIG (que faz a tara) e espera o ESP confirmar o modo de calibração."""

    confirmed = Signal()     # ESP entrou no modo de calibração
    error     = Signal(str)  # mensagem de erro

    def __init__(self, com: fact_cli.Receiver):
        super().__init__()
        self.com = com

    def run(self):
        try:
            confirmed = fact_cli.enter_config_mode(self.com)
        except Exception as e:
            self.error.emit(str(e))
            return

        if confirmed:
            self.confirmed.emit()
        else:
            self.error.emit('ESP não confirmou modo de calibração. Verifique a conexão.')


class SamplerThread(QThread):
    """Coleta amostras da serial em background, sem travar a UI.

    O ESP já precisa estar no modo de calibração (ver ConfigModeThread).
    """

    progress        = Signal(int)   # 0–100
    sample_received = Signal(int)   # valor de cada amostra
    finished        = Signal(list)  # lista final de amostras
    error           = Signal(str)   # mensagem de erro

    def __init__(self, com: fact_cli.Receiver, n_samples: int = 100):
        super().__init__()
        self.com = com
        self.n_samples = n_samples
        self._running = True

    def run(self):
        samples = []
        sample_count = 0

        # Descarta o que chegou enquanto o peso era colocado na célula
        self.com.serial.reset_input_buffer()

        try:
            while self._running and self.com.check_connection() and sample_count < self.n_samples:
                raw = self.com.read_response()
                print(f"raw={repr(raw)}")
                value = fact_cli.try_parse_int(raw)
                if value is None:
                    continue
                samples.append(value)
                sample_count += 1
                self.sample_received.emit(value)
                self.progress.emit(int(sample_count / self.n_samples * 100))
        except Exception as e:
            self.error.emit(str(e))
            return

        self.finished.emit(samples)

    def stop(self):
        self._running = False


class Calibrator:

    def __init__(self):
        self.app = QApplication(sys.argv)
        self.window = QMainWindow()
        self.ui = fact_ui.Ui_MainWindow()
        self.logger = logging.getLogger('Calibrator')
        self.ui.setupUi(self.window)
        self.stopper = extend_ui(self.ui)
        self.window.show()
        self.app.exec()
        self.stopper()  # para a thread antes de destruir tudo


def extend_ui(ui):

    dummy_elements = False
    state = {
        'com': None,
        'thread': None,
    }

    def update_port_list():
        ports = fact_cli.list_ports()
        ui.cbox_seriais.clear()
        ui.cbox_seriais.addItems(['dummy-data-4', 'dummy-data-5', 'dummy-data-6']) if dummy_elements else None
        ui.cbox_seriais.addItems(ports)

    def connect_esp():
        port = str(ui.cbox_seriais.currentText())
        print(f"{port=}")
        try:
            state['com'] = fact_cli.Receiver(port)
        except Exception as e:
            ui.display_status.setText(f'Erro: {e}')
            return
        state['com'].serial.reset_input_buffer()
        print(f"com={state['com']}")
        ui.display_status.setText('Conectado')

    def send_factor():
        com = state['com']
        print(f"{com=}")
        if com is None:
            ui.display_status.setText('Desconectado')
            return
        fator = ui.line_edit_fator.text().strip().replace(',', '.')
        comma = f'SET LOAD FACTOR {fator}\n'
        print(f"-> {comma.strip()}")
        com.serial.reset_input_buffer()
        com.serial.write(comma.encode('utf-8'))
        com.serial.flush()
        for _ in range(10):
            line = com.read_response()
            if line:
                print(line)

    def on_sample_received(value):
        print(f"sample: {value}")

    def on_progress(value):
        ui.progressBar.setValue(value)

    def on_finished(samples):
        print(f"Coleta finalizada: {len(samples)} amostras")
        ui.btn_calcular.setEnabled(True)
        ui.display_status.setText('Conectado')

        expected_weight_str = ui.lineEdit_4.text()
        if not expected_weight_str:
            ui.display_status.setText('Informe o Peso Real')
            return
        try:
            expected_weight = float(expected_weight_str.replace(',', '.'))
        except ValueError:
            ui.display_status.setText('Peso Real inválido')
            return

        if not samples:
            ui.display_status.setText('Nenhuma amostra recebida')
            return

        calibration_factor = fact_cli.get_calibration_factor(samples, expected_weight)
        ui.line_edit_fator.setText(str(calibration_factor))

    def on_error(msg):
        print(f"Erro na thread: {msg}")
        ui.display_status.setText(f'Erro: {msg}')
        ui.btn_calcular.setEnabled(True)

    def calculate_factor():
        com = state['com']
        print(f"{com=}")

        if com is None:
            ui.display_status.setText('Desconectado')
            return

        expected_weight_str = ui.lineEdit_4.text()
        if not expected_weight_str:
            ui.display_status.setText('Informe o Peso Real')
            return

        if state['thread'] is not None and state['thread'].isRunning():
            return

        # 1) A tara (feita pelo INIT CONFIG) precisa ser feita SEM peso na célula
        QMessageBox.information(
            None, 'Calibração',
            'Retire todo o peso da célula de carga e clique em OK.\n'
            'O ESP vai fazer a tara (zerar) agora.')

        ui.btn_calcular.setEnabled(False)
        ui.display_status.setText('Fazendo a tara...')
        ui.progressBar.setValue(0)

        thread = ConfigModeThread(com)
        thread.confirmed.connect(on_config_mode)
        thread.error.connect(on_error)
        state['thread'] = thread
        thread.start()

    def on_config_mode():
        # 2) Com a tara feita, o peso de referência pode ser colocado
        QMessageBox.information(
            None, 'Calibração',
            f'Tara concluída.\n\nColoque o peso de {ui.lineEdit_4.text()} sobre a '
            'célula de carga, espere estabilizar e clique em OK.')

        ui.display_status.setText('Coletando amostras...')

        thread = SamplerThread(state['com'], n_samples=200)
        thread.progress.connect(on_progress)
        thread.sample_received.connect(on_sample_received)
        thread.finished.connect(on_finished)
        thread.error.connect(on_error)
        state['thread'] = thread
        thread.start()

    def stop_all():
        if state['thread'] is not None and state['thread'].isRunning():
            state['thread'].stop()
            state['thread'].wait()

    ports = fact_cli.list_ports()
    ui.cbox_seriais.addItems(['dummy-data-1', 'dummy-data-2', 'dummy-data-3']) if dummy_elements else None
    ui.cbox_seriais.addItems(ports)

    ui.btn_atualizar.clicked.connect(update_port_list)
    ui.btn_calcular.clicked.connect(calculate_factor)
    ui.btn_conectar.clicked.connect(connect_esp)
    ui.btn_enviar.clicked.connect(send_factor)

    return stop_all


if __name__ == '__main__':
    Calibrator()
