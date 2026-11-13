# Explicação do Sistema

Este sistema é composto pelos seguintes componentes principais:

1. **`firmware/firmware.ino`**: roda no ESP32 e transmite leituras (RAW e em unidades) via Serial.
2. **`software/config-load-cell/fator_de_calibracao/fator_de_calibracao_main.py`**: ferramenta GUI para calcular/enviar o fator de calibração.
3. **`software/config-load-cell/fator_de_calibracao/fator_de_calibracao_cli.py`**: utilitários/CLI de calibração (opcional).

- **Firmware**: coleta dados da célula de carga (pelo ADC ADS1256) e envia via Serial/Bluetooth.
- **GUI**: lê as amostras RAW do ESP32, calcula o fator e envia via comando `SET LOAD FACTOR`.

## Passo a Passo para Configurar uma Célula do Zero

1. **Montagem do Hardware**:

   - Conecte a célula de carga ao módulo ADS1256 (`AIN0`/`AIN1`).
   - Conecte o módulo ADS1256 ao ESP32 via SPI (ver [`docs/HARDWARE.md`](../../docs/HARDWARE.md)).

2. **Configuração do ESP32**:

    - Carregue o código `firmware/firmware.ino` no ESP32 usando a IDE Arduino.
   - Verifique se o ESP32 está enviando dados da célula de carga via comunicação serial.

3. **Calibração da Célula de Carga**:

    - Coloque um peso conhecido na célula.
    - Execute a GUI: `python software/config-load-cell/fator_de_calibracao/fator_de_calibracao_main.py`
    - Selecione a porta serial, clique em "Conectar" e depois em "Calcular".
    - Confirme/ajuste o valor e clique em "Enviar" para persistir no ESP32.

4. **Verificação Final**:
    - Reinicie o ESP32 e valide a leitura em `kgf` via Serial (o firmware faz log contínuo).

Obs.: os arquivos legados (`Config.ino`, `Verify.ino`, `DadosSerial.py`, `Calibrar.py` e os dados brutos do HX711) foram movidos para [`Legacy/software/config-load-cell/`](../../Legacy/software/config-load-cell/) e não fazem parte do fluxo atual.
