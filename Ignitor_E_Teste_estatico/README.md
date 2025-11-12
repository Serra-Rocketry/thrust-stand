# Ignitor e Teste Estático

Este projeto utiliza um ESP32 para controlar a ignição de um motor de foguete e realizar testes estáticos, registrando dados de empuxo em um cartão SD. A comunicação pode ser feita via Serial (USB) ou Bluetooth.

## Componentes Utilizados

- ESP32
- Relé
- Botão
- LED
- Cartão SD
- Módulo RTC DS3231
- Célula de carga HX711
- Módulo Bluetooth

## Bibliotecas Utilizadas

- `Wire.h`
- `RTClib.h`
- `HX711.h`
- `FS.h`
- `SD.h`
- `SPI.h`
- `Pushbutton.h`
- `BluetoothSerial.h`

## Definições de Pinos

- `RELE_PIN`: Pino de controle do relé (25)
- `BTN_PIN`: Pino do botão (33)
- `LED_PIN`: Pino do LED (4)
- `CS_PIN`: Pino do cartão SD (5)
- `CELULA_DT_PIN`: Pino de dados da célula de carga (26)
- `CELULA_SCK_PIN`: Pino de clock da célula de carga (27)
- `SELECT_PIN`: Pino de seleção de modo (32)

## Modos de Operação

- **Ignitor e Teste Estático**: Realiza a ignição e registra dados de empuxo.
- **Apenas Ignitor**: Apenas realiza a ignição.

## Comandos

- `A`: Aguardando comando
- `C`: Iniciar contagem
- `D`: Parar de salvar dados
- `E`: Tarar célula de carga
- `1`: Ativar relé
- `0`: Desativar relé

## Funções Principais

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

## Como Usar

1. Conecte os componentes conforme os pinos definidos.
2. Carregue o código no ESP32.
3. Utilize a comunicação Serial ou Bluetooth para enviar comandos.
4. Selecione o modo de operação através do pino `SELECT_PIN`.

## Observações

- Certifique-se de que o cartão SD está corretamente inserido.
- Ajuste o fator de calibração da célula de carga conforme necessário.
