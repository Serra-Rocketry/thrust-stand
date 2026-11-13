// INCLUSÃO DE BIBLIOTECAS
#include <FS.h>
#include <SD.h>
#include <LittleFS.h>
#include <SPI.h>
#include <Pushbutton.h>
#include <BluetoothSerial.h>
#include <Preferences.h>

#include "ADS1256.h"
#include "Pressure.h"

// Definições de pinos e constantes
#define LED_PIN 19        // Pino do LED
#define CS_PIN 5          // Pino do cartão SD (CS)
#define SD_SCK_PIN 25     // Pino SCK do SD
#define SD_MISO_PIN 23    // Pino MISO do SD
#define SD_MOSI_PIN 27    // Pino MOSI do SD
#define BUZZER_PIN 32     // Pino do buzzer
#define BTN_PIN 33        // Pino do botão
#define ADS_SCLK_PIN 18   // ADS1256 SCLK (reaproveita a trilha do antigo SCK do HX711)
#define ADS_DOUT_PIN 4    // ADS1256 DOUT -> MISO (reaproveita a trilha do antigo DT do HX711)
#define ADS_DIN_PIN 13    // ADS1256 DIN  <- MOSI
#define ADS_CS_PIN 26     // ADS1256 CS
#define ADS_DRDY_PIN 34   // ADS1256 DRDY (pino só de entrada)
#define TAXA_AMOSTRAGEM_HZ 500 // Amostras por segundo (empuxo + pressão)
#define PERIODO_US (1000000UL / TAXA_AMOSTRAGEM_HZ)
#define TRANSMISSAO_HZ 50      // Linhas por segundo enviadas por Serial/BT (o SD recebe todas)
#define DECIMACAO (TAXA_AMOSTRAGEM_HZ / TRANSMISSAO_HZ)
#define FILA_TAMANHO 1024      // Amostras na fila até o SD (~2 s a 500 Hz)
#define FLUSH_INTERVALO_MS 1000 // Força a escrita no cartão a cada 1 s
#define TARE_AMOSTRAS 50  // Leituras usadas na média da tara
#define NVS_CHAVE_FATOR "fatorADS1256" // Chave do fator de calibração no NVS

// Instância SPI customizada para o SD (SCK=25, MISO=23, MOSI=27, CS=5)
SPIClass spiSD(VSPI);

// Barramento SPI separado para o ADS1256 (SCLK=18, MISO=4, MOSI=13)
SPIClass spiADC(HSPI);

// Variáveis globais
const float VinPressure = 5.0;    // Tensão que alimenta o sensor
const float VminPressure = 0.27;  // Tensão de saída em 0 MPa
const float VmaxPressure = 4.5;   // Tensão de saída em 10 MPa
const float maxPressure = 10.0;   // Pressão máxima do sensor em MPa
const float R1 = 2200.0;          // Resistor entre sensor e entrada AIN2 do ADS1256
const float R2 = 3300.0;          // Resistor entre AIN2 e GND
const float ADS_VREF = 2.5;       // Referência de tensão do módulo ADS1256

// Célula de carga SZ51X-500kg (certificado de calibração)
const float CELULA_SENSIBILIDADE = 0.002; // 2 mV/V
const float CELULA_EXCITACAO = 5.0;       // Tensão de excitação (V)
const float CELULA_CAPACIDADE = 500.0;    // Capacidade (kg)

// Fator teórico (contagens do ADC por kg), usado até a primeira calibração:
// (V por kg na saída da célula) x (contagens por V no PGA 64)
// = (0,002 x 5 / 500) x (8388607 / (2 x 2,5 / 64)) ~ 2147,5
const float FATOR_TEORICO = (CELULA_SENSIBILIDADE * CELULA_EXCITACAO / CELULA_CAPACIDADE)
                          * (8388607.0 / (2.0 * ADS_VREF / 64.0));
float maxValues[2] = {0.0, 0.0};  // Leituras de pico (peso, pressão)
unsigned long proximaAmostraUs = 0; // Instante (micros) da próxima amostra
float loadFactor = 0.0;           // Fator de calibração
long tareOffset = 0;              // Leitura bruta da célula sem carga (tara)
String filedir = "";              // Caminho do arquivo

// Uma linha do arquivo de dados, passada do núcleo de aquisição para o de gravação
struct Amostra
{
  uint32_t ms;
  float empuxo;
  float pressao;
};

QueueHandle_t filaAmostras = NULL;      // Fila aquisição (núcleo 1) -> gravação (núcleo 0)
File dataFile;                          // Arquivo de dados, aberto durante todo o teste
volatile uint32_t amostrasPerdidas = 0; // Amostras descartadas por fila cheia

enum StorageType { STORAGE_NONE, STORAGE_SD, STORAGE_LITTLEFS };
StorageType storageType = STORAGE_NONE;

static bool configMode = false;

// Instanciação de objetos
Pushbutton button(BTN_PIN);
PressureSensor pressureSensor(R1, R2, VminPressure, VmaxPressure, maxPressure);
ADS1256 adc(spiADC, ADS_CS_PIN, ADS_DRDY_PIN, ADS_VREF);
BluetoothSerial SerialBT;

// Célula de carga: entrada diferencial AIN0(+)/AIN1(-), ganho 64
// (sinal de ~10 mV a plena carga: 2 mV/V com excitação de 5 V)
ADS1256Channel canalCelula = {ADS1256_AIN0, ADS1256_AIN1, ADS1256_PGA_64};

// Sensor de pressão: AIN2 contra AINCOM (GND), ganho 1
// (até 2,7 V depois do divisor; com o buffer ligado, o limite é 3 V)
ADS1256Channel canalPressao = {ADS1256_AIN2, ADS1256_AINCOM, ADS1256_PGA_1};
Preferences preferences;

// Declarações antecipadas
void processCommand(String command);
void setLoadFactor(float factor);
void buzzSignal(String signal);
bool setupStorage();
bool setupStorageAndFile();
bool setupADS1256();
bool readLoadCellRaw(long &raw);
bool tareLoadCell(int amostras);
float rawToKg(long raw);
bool readPressureMPa(float &pressao);
void handleTare();
void logData(unsigned long ms);
bool startRecording();
void recordingTask(void *param);
bool writeFile(const String &path, const String &message);
void printToSerials(const String &message);
void staticTest();
String generateFileName();

void setup()
{
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT");

  preferences.begin("app", false);
  // Chave nova: a antiga ("loadFactor") guardava o fator do HX711, que não vale para o ADS1256
  loadFactor = preferences.getFloat(NVS_CHAVE_FATOR, FATOR_TEORICO);

  Serial.println("=== BOOT ===");
  printToSerials("loadFactor carregado: " + String(loadFactor, 4));

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT);

  bool sdOk = setupStorageAndFile() && startRecording();
  bool adcOk = setupADS1256();

  if (sdOk && adcOk)
  {
    printToSerials("Sistema configurado. Transmitindo...");
    buzzSignal("Sucesso");
  }
  else
  {
    if (!sdOk)  printToSerials("ERRO: Falha no storage.");
    if (!adcOk) printToSerials("ERRO: Falha no ADS1256.");
    printToSerials("Falha critica. Reiniciando...");
    buzzSignal("Alerta");
    delay(3000);
    ESP.restart();
  }
}

void loop()
{
  if (!configMode)
  {
    staticTest();
  }

  if (button.getSingleDebouncedPress())
  {
    handleTare();
  }

  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }

  if (SerialBT.available())
  {
    String command = SerialBT.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }

  // Stream RAW de calibração
  if (configMode)
  {
    long raw;
    if (readLoadCellRaw(raw))
    {
      long valor = raw - tareOffset; // leitura bruta já descontada a tara
      Serial.println(valor);
      SerialBT.println(valor);
    }
    delay(200);
  }
}

void processCommand(String command)
{
  if (command.equalsIgnoreCase("TARE"))
  {
    handleTare();
    return;
  }

  if (command.equalsIgnoreCase("GET LOAD FACTOR"))
  {
    printToSerials("loadFactor atual: " + String(loadFactor, 4));
    return;
  }

  if (command.startsWith("INIT CONFIG"))
  {
    if (tareLoadCell(TARE_AMOSTRAS))
    {
      configMode = true;
      printToSerials("Modo calibracao ativo. Aguardando SET LOAD FACTOR...");
    }
    else
    {
      printToSerials("ADS1256 nao respondeu. Verifique conexoes.");
    }
    return;
  }

  if (configMode && command.startsWith("SET LOAD FACTOR"))
  {
    int lastSpaceIndex = command.lastIndexOf(' ');
    if (lastSpaceIndex != -1)
    {
      String factorStr = command.substring(lastSpaceIndex + 1);
      float factor = factorStr.toFloat();
      if (!isnan(factor) && factor != 0.0)
      {
        setLoadFactor(factor);
        configMode = false;
        printToSerials("Modo calibracao finalizado.");
      }
      else
      {
        printToSerials("Valor invalido. Tente novamente.");
        buzzSignal("Alerta");
      }
    }
    return;
  }
}

void setLoadFactor(float factor)
{
  loadFactor = factor;
  preferences.putFloat(NVS_CHAVE_FATOR, loadFactor);
  printToSerials("Fator de carga atualizado: " + String(loadFactor, 4));
  buzzSignal("Sucesso");
}

void buzzSignal(String signal)
{
  int frequency = 1000;
  if (signal == "Alerta")
  {
    for (int i = 0; i < 5; i++)
    {
      tone(BUZZER_PIN, frequency, 200);
      delay(350);
    }
  }
  else if (signal == "Sucesso")
  {
    for (int i = 0; i < 3; i++)
    {
      tone(BUZZER_PIN, frequency, 100);
      delay(200);
    }
  }
  else if (signal == "Ativado")
  {
    tone(BUZZER_PIN, frequency, 500);
  }
  else if (signal == "Beep")
  {
    tone(BUZZER_PIN, frequency, 50);
  }
  else
  {
    Serial.println("Sinal invalido!");
  }
}

String generateFileName()
{
  const char* prefix = (storageType == STORAGE_SD) ? "Dados" : "dados";

  for (int i = 1; i <= 999; i++)
  {
    char candidate[32];
    snprintf(candidate, sizeof(candidate), "/%s_%03d.txt", prefix, i);

    bool exists = false;
    if (storageType == STORAGE_SD) {
      exists = SD.exists(candidate);
    } else if (storageType == STORAGE_LITTLEFS) {
      exists = LittleFS.exists(candidate);
    }

    if (!exists)
      return String(candidate);
  }

  return "/overflow.txt";
}

bool setupStorage()
{
  Serial.println("Inicializando SD...");
  spiSD.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, CS_PIN);
  delay(100); // aguarda estabilização do SPI

  if (SD.begin(CS_PIN, spiSD) && SD.cardType() != CARD_NONE)
  {
    storageType = STORAGE_SD;
    Serial.println("SD iniciado!");
    Serial.printf("Tipo: %s\n",
      SD.cardType() == CARD_MMC  ? "MMC"  :
      SD.cardType() == CARD_SD   ? "SDSC" :
      SD.cardType() == CARD_SDHC ? "SDHC" : "UNKNOWN");
    return true;
  }

  Serial.println("SD falhou. Tentando LittleFS...");
  yield();

  if (LittleFS.begin(true))
  {
    storageType = STORAGE_LITTLEFS;
    Serial.println("LittleFS iniciado!");
    Serial.printf("Total: %u bytes, Usado: %u bytes\n",
      (unsigned)LittleFS.totalBytes(), (unsigned)LittleFS.usedBytes());
    return true;
  }

  Serial.println("ERRO: Nenhum storage disponivel!");
  return false;
}

bool setupStorageAndFile()
{
  if (!setupStorage())
    return false;

  filedir = generateFileName();
  Serial.print("Arquivo: ");
  Serial.println(filedir);

  if (!writeFile(filedir, "Tempo,Empuxo,Pressao\n"))
  {
    printToSerials("Falha ao criar arquivo");
    return false;
  }

  printToSerials("Arquivo criado: " + filedir);
  return true;
}

bool setupADS1256()
{
  Serial.println("Iniciando ADS1256...");
  spiADC.begin(ADS_SCLK_PIN, ADS_DOUT_PIN, ADS_DIN_PIN, -1); // CS controlado pelo driver
  delay(100); // aguarda o oscilador do módulo estabilizar

  if (!adc.begin(ADS1256_DRATE_2000, true))
  {
    Serial.println("ERRO: ADS1256 nao respondeu. Verifique SCLK=18, DOUT=4, DIN=13, CS=26, DRDY=34.");
    return false;
  }
  Serial.println("ADS1256 detectado OK.");

  if (!adc.calibrate(canalCelula) || !adc.calibrate(canalPressao))
  {
    Serial.println("ERRO: Falha na autocalibracao do ADS1256.");
    return false;
  }

  Serial.print("Fator de carga: ");
  Serial.println(loadFactor, 4);

  // Warmup: descarta as primeiras leituras para estabilizar
  Serial.println("Aguardando estabilizacao...");
  long raw;
  for (int i = 0; i < 10; i++)
  {
    readLoadCellRaw(raw);
    delay(100);
  }

  if (!tareLoadCell(TARE_AMOSTRAS))
  {
    Serial.println("ERRO: Falha na tara inicial.");
    return false;
  }
  Serial.println("tare OK");

  printToSerials("ADS1256 conectado");
  return true;
}

// Lê a célula de carga (valor bruto de 24 bits, sem tara)
bool readLoadCellRaw(long &raw)
{
  int32_t valor;
  if (!adc.read(canalCelula, valor, 1000))
    return false;
  raw = valor;
  return true;
}

// Zera a célula: guarda a média de várias leituras sem carga
bool tareLoadCell(int amostras)
{
  int64_t soma = 0;
  long raw;
  for (int i = 0; i < amostras; i++)
  {
    if (!readLoadCellRaw(raw))
      return false;
    soma += raw;
  }
  tareOffset = soma / amostras;
  return true;
}

// Converte a leitura bruta em kg usando a tara e o fator de calibração
float rawToKg(long raw)
{
  return (raw - tareOffset) / loadFactor;
}

// Lê o sensor de pressão pelo ADS1256 e converte para MPa
bool readPressureMPa(float &pressao)
{
  int32_t valor;
  if (!adc.read(canalPressao, valor, 1000))
    return false;
  pressao = pressureSensor.toMPa(adc.toVolts(valor, canalPressao.pga));
  return true;
}

void handleTare()
{
  if (tareLoadCell(TARE_AMOSTRAS))
  {
    buzzSignal("Beep");
    printToSerials("Celula Zerada!");
  }
  else
  {
    printToSerials("ADS1256 nao respondeu, TARE ignorado.");
  }
}

void logData(unsigned long ms)
{
  long raw;
  if (!readLoadCellRaw(raw))
  {
    Serial.println("AVISO: ADS1256 timeout em logData, pulando leitura.");
    return;
  }

  float pressao;
  if (!readPressureMPa(pressao))
  {
    Serial.println("AVISO: ADS1256 timeout na pressao, pulando leitura.");
    return;
  }

  Amostra amostra;
  amostra.ms = ms;
  amostra.empuxo = rawToKg(raw); // o sinal é acertado pelo fator de calibração
  amostra.pressao = pressao;

  // Não espera: se a fila estiver cheia (SD travado por muito tempo), conta a perda
  // (só este núcleo escreve em amostrasPerdidas; o outro apenas lê)
  if (xQueueSend(filaAmostras, &amostra, 0) != pdTRUE)
    amostrasPerdidas = amostrasPerdidas + 1;
}

// Abre o arquivo de dados (que fica aberto) e cria a tarefa de gravação
bool startRecording()
{
  if (storageType == STORAGE_SD) {
    dataFile = SD.open(filedir, FILE_APPEND);
  } else if (storageType == STORAGE_LITTLEFS) {
    dataFile = LittleFS.open(filedir, FILE_APPEND);
  }

  if (!dataFile)
  {
    printToSerials("Falha ao abrir arquivo para gravacao: " + filedir);
    return false;
  }

  filaAmostras = xQueueCreate(FILA_TAMANHO, sizeof(Amostra));
  if (filaAmostras == NULL)
  {
    printToSerials("Falha ao criar a fila de amostras");
    return false;
  }

  // Núcleo 0: o loop() (aquisição) roda no núcleo 1
  if (xTaskCreatePinnedToCore(recordingTask, "gravacao", 8192, NULL, 1, NULL, 0) != pdPASS)
  {
    printToSerials("Falha ao criar a tarefa de gravacao");
    return false;
  }

  return true;
}

// Tarefa do núcleo 0: grava cada amostra da fila no arquivo (que fica
// aberto) e transmite 1 a cada DECIMACAO por Serial/BT. Assim, uma demora
// do cartão SD não atrasa a leitura dos sensores, que roda no núcleo 1.
void recordingTask(void *param)
{
  Amostra amostra;
  char linha[64];
  unsigned int contadorTransmissao = 0;
  unsigned long ultimoFlush = millis();
  uint32_t perdidasAvisadas = 0;
  bool gravacaoOk = true;

  for (;;)
  {
    if (xQueueReceive(filaAmostras, &amostra, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      int tamanho = snprintf(linha, sizeof(linha), "%lu,%.6f,%.6f\n",
                             (unsigned long)amostra.ms, amostra.empuxo, amostra.pressao);

      bool ok = dataFile.write((const uint8_t *)linha, tamanho) == (size_t)tamanho;
      if (ok != gravacaoOk)
      {
        gravacaoOk = ok;
        digitalWrite(LED_PIN, ok ? HIGH : LOW);
        if (!ok) printToSerials("Falha ao gravar linha no arquivo");
      }

      if (amostra.empuxo > maxValues[0]) maxValues[0] = amostra.empuxo;
      if (amostra.pressao > maxValues[1]) maxValues[1] = amostra.pressao;

      // A Serial a 115200 baud não comporta 500 linhas/s
      if (++contadorTransmissao >= DECIMACAO)
      {
        contadorTransmissao = 0;
        linha[tamanho - 1] = '\0'; // tira o '\n' (println já coloca)
        printToSerials(String(linha));
      }
    }

    // O buffer do arquivo só vai para o cartão no flush: perde no máximo
    // FLUSH_INTERVALO_MS de dados se a energia cair
    if (millis() - ultimoFlush >= FLUSH_INTERVALO_MS)
    {
      dataFile.flush();
      ultimoFlush = millis();

      if (amostrasPerdidas != perdidasAvisadas)
      {
        perdidasAvisadas = amostrasPerdidas;
        printToSerials("AVISO: amostras perdidas (fila cheia): " + String(perdidasAvisadas));
      }
    }
  }
}

bool writeFile(const String &path, const String &message)
{
  File file;

  if (storageType == STORAGE_SD) {
    file = SD.open(path, FILE_WRITE);
  } else if (storageType == STORAGE_LITTLEFS) {
    file = LittleFS.open(path, FILE_WRITE);
  } else {
    return false;
  }

  if (!file)
  {
    printToSerials("Falha ao abrir arquivo para escrita: " + path);
    return false;
  }

  bool ok = file.print(message);
  file.close();

  if (ok)
  {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("SD: arquivo criado OK");
  }
  else
  {
    printToSerials("Falha ao escrever no arquivo");
    digitalWrite(LED_PIN, LOW);
  }

  return ok;
}

void printToSerials(const String &message)
{
  Serial.println(message);
  SerialBT.println(message);
}

// Dispara uma amostra a cada PERIODO_US, mantendo a cadência fixa
void staticTest()
{
  unsigned long agora = micros();
  if ((long)(agora - proximaAmostraUs) < 0)
    return;

  proximaAmostraUs += PERIODO_US;
  // Se atrasou mais de um período (ex.: durante uma TARE), não tenta
  // compensar com uma rajada de leituras: retoma a cadência a partir de agora
  if ((long)(agora - proximaAmostraUs) >= 0)
    proximaAmostraUs = agora + PERIODO_US;

  logData(millis());
}
