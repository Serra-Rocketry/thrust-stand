# Serra Rocketry - Dados de Teste Estático

Este repositório contém diversos projetos relacionados ao controle e monitoramento de testes estáticos de motores de foguete utilizando ESP32 e outros componentes eletrônicos. Abaixo estão descritos os principais projetos e suas funcionalidades.

## Projetos

### 1. Ignitor e Teste Estático

Este projeto utiliza um ESP32 para controlar a ignição de um motor de foguete e realizar testes estáticos, registrando dados de empuxo em um cartão SD. A comunicação pode ser feita via Serial (USB) ou Bluetooth.

#### Componentes Utilizados

- ESP32
- Relé
- Botão
- LED
- Cartão SD
- Módulo RTC DS3231
- Célula de carga HX711
- Módulo Bluetooth

#### Bibliotecas Utilizadas

- `Wire.h`
- `RTClib.h`
- `HX711.h`
- `FS.h`
- `SD.h`
- `SPI.h`
- `Pushbutton.h`
- `BluetoothSerial.h`

#### Definições de Pinos

- `RELE_PIN`: Pino de controle do relé (25)
- `BTN_PIN`: Pino do botão (33)
- `LED_PIN`: Pino do LED (4)
- `CS_PIN`: Pino do cartão SD (5)
- `CELULA_DT_PIN`: Pino de dados da célula de carga (26)
- `CELULA_SCK_PIN`: Pino de clock da célula de carga (27)
- `SELECT_PIN`: Pino de seleção de modo (32)

#### Modos de Operação

- **Ignitor e Teste Estático**: Realiza a ignição e registra dados de empuxo.
- **Apenas Ignitor**: Apenas realiza a ignição.

#### Comandos

- `A`: Aguardando comando
- `C`: Iniciar contagem e salvar dados
- `D`: Parar de salvar dados
- `E`: Tarar célula de carga
- `1`: Ativar relé
- `0`: Desativar relé

#### Funções Principais

- `setup()`: Configura os pinos e inicializa os módulos.
- `loop()`: Seleciona o modo de operação e executa as funções correspondentes.
- `setupRTC()`: Inicializa o módulo RTC.
- `setupSDCard()`: Inicializa o cartão SD e cria diretórios e arquivos.
- `setupHX711()`: Inicializa a célula de carga.
- `handleButtonPress()`: Trata o pressionamento do botão.
- `logData()`: Registra dados no cartão SD.
- `getCurrentDate()`: Retorna a data atual.
- `getCurrentDateTime()`: Retorna a data e hora atuais.
- `writeFile()`: Escreve dados em um arquivo.
- `appendFile()`: Adiciona dados em um arquivo.
- `createDir()`: Cria um diretório.
- `printToSerials()`: Imprime mensagens na Serial e Serial Bluetooth.
- `ign_estatico()`: Função de ignição e teste estático.
- `ign()`: Função de ignição.

### 2. Calibrar Célula de Carga

Este sistema é composto por quatro componentes principais para calibrar uma célula de carga:

1. **Config.ino**: Código para ESP32 que envia leituras da célula de carga.
2. **DadosSerial.py**: Script que lê as leituras da célula de carga enviadas pelo ESP32.
3. **Calibrar.py**: Script que calcula o fator de calibração da célula de carga.
4. **Verify.ino**: Código que verifica a precisão da célula de carga após a calibração.

#### Passo a Passo para Configurar uma Célula do Zero

1. **Montagem do Hardware**:
    - Conecte a célula de carga ao amplificador HX711.
    - Conecte o amplificador HX711 ao ESP32.

2. **Configuração do ESP32**:
    - Carregue o código `Config.ino` no ESP32 usando a IDE Arduino.
    - Verifique se o ESP32 está enviando dados da célula de carga via comunicação serial.

3. **Leitura dos Dados**:
    - Adicione um peso conhecido na célula de carga.
    - Execute o script `DadosSerial.py` para ler os dados enviados pelo ESP32.
    - Certifique-se de que os dados estão sendo recebidos corretamente.

4. **Calibração da Célula de Carga**:
    - Verifique o arquivo `Dados.txt`.
    - Execute o script `Calibrar.py` para calcular o fator de calibração.
    - Informe o peso usado anteriormente.

5. **Verificação Final**:
    - Carregue o código `Verify.ino` no ESP32 para verificar a precisão da célula de carga.
    - Teste a célula de carga com pesos conhecidos para garantir a precisão.
    - Faça ajustes adicionais no fator de calibração, se necessário.

## Contribuição

1. Faça um fork do projeto.
2. Crie uma branch para sua feature (`git checkout -b feature/nova-feature`).
3. Commit suas mudanças (`git commit -am 'Adiciona nova feature'`).
4. Faça push para a branch (`git push origin feature/nova-feature`).
5. Abra um Pull Request.
=======
# Serra Rocketry Static Test Stand

This repository contains the code and documentation for the Serra Rocketry team's static test stand, used to measure the thrust of rocket motors. The tester is written in C++ for an ESP32 or ESP8266 microcontroller(arduino compatible boards) .

## Bill of Material (BOM)

* ESP32 or ESP8266
    to read sensor data and send to the server, since it is cheap and will be near the motor combustion.
* Raspberry Pi or similar 
    To be a server to get and display the data in real time
* Load Cell
    Depending on the motor force, tipicaly, the precision is about 10%-15% of the max load cell capacity.
* V-slots
    To build a box to hold the load cel, the motor and other sensors.

## Installation and Setup

To use the load cell tester, follow these steps:

1. Clone the repository.
2. Connect the load cell and other sensor to the ESP.
3. Connect the ESP to your computer.
4. Open the `code/LoadCellTester/LoadCellTester.ino` sketch in the Arduino IDE.
5. Upload the sketch to the Arduino.
6. Open the serial monitor to view the output from the tester.

## InfluxDB Setup

To store the data from the load cell tester, you'll need to set up an InfluxDB server. Ideally, you should use a Raspberry Pi or similar device as your server. Follow the instructions in the [Getting Started with Arduino and InfluxDB](https://www.influxdata.com/blog/getting-started-arduino-influxdb/) blog post to set up InfluxDB on your server.

It's important to ensure that the date and time are correct on the InfluxDB server in order for the data to be stored correctly. To do this, you should set up an NTP server on the InfluxDB server to synchronize the clock. This will ensure that the data is stored with the correct timestamp.

## Usage

Here's how to use the load cell tester:

1. Place the rocket motor on the load cell.
2. Apply power to the motor.
3. The tester will output the thrust of the motor in Newtons to the serial monitor.
4. The data will also be stored in your InfluxDB server.


## Contributing

We welcome contributions from the community! To contribute, follow these steps:

1. Fork the repository.
2. Make your changes.
3. Create a pull request.


## References
* https://www.influxdata.com/blog/getting-started-arduino-influxdb/

