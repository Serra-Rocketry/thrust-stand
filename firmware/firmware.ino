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
#define PRESSURE_PIN 35   // Pino do sensor de pressão
#define INTERVALO 1       // Intervalo de leitura em milissegundos
#define TARE_AMOSTRAS 50  // Leituras usadas na média da tara

// Instância SPI customizada para o SD (SCK=25, MISO=23, MOSI=27, CS=5)
SPIClass spiSD(VSPI);

// Barramento SPI separado para o ADS1256 (SCLK=18, MISO=4, MOSI=13)
SPIClass spiADC(HSPI);

// Variáveis globais
const float VinPressure = 5.0;    // Tensão que alimenta o sensor
const float VminPressure = 0.27;  // Tensão de saída em 0 MPa
const float VmaxPressure = 4.5;   // Tensão de saída em 10 MPa
const float maxPressure = 10.0;   // Pressão máxima do sensor em MPa
const float R1 = 2200.0;          // Resistor entre sensor e pino ESP32
const float R2 = 3300.0;          // Resistor entre pino ESP32 e GND
const int RESOLUCAO_ADC = 4095;   // ESP32 tem ADC de 12 bits (2^12 - 1)
const float TENSAO_MAX_ADC = 3.3; // Tensão de referência do ADC do ESP32
const float ADS_VREF = 2.5;       // Referência de tensão do módulo ADS1256
float maxValues[2] = {0.0, 0.0};  // Leituras de pico (peso, pressão)
unsigned long previousMillis = 0; // Controle de tempo
float loadFactor = 0.0;           // Fator de calibração
long tareOffset = 0;              // Leitura bruta da célula sem carga (tara)
String filedir = "";              // Caminho do arquivo
String leitura = "";              // Leitura dos dados

enum StorageType { STORAGE_NONE, STORAGE_SD, STORAGE_LITTLEFS };
StorageType storageType = STORAGE_NONE;

static bool configMode = false;

// Instanciação de objetos
Pushbutton button(BTN_PIN);
PressureSensor pressureSensor(PRESSURE_PIN, R1, R2, RESOLUCAO_ADC, TENSAO_MAX_ADC, VminPressure, VmaxPressure, maxPressure);
ADS1256 adc(spiADC, ADS_CS_PIN, ADS_DRDY_PIN, ADS_VREF);
BluetoothSerial SerialBT;

// Célula de carga: entrada diferencial AIN0(+)/AIN1(-), ganho 64
// (sinal de ~10 mV a plena carga: 2 mV/V com excitação de 5 V)
ADS1256Channel canalCelula = {ADS1256_AIN0, ADS1256_AIN1, ADS1256_PGA_64};
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
void handleTare();
void logData(unsigned long ms);
bool writeFile(const String &path, const String &message);
void appendFile(const String &path, const String &message);
void printToSerials(const String &message);
void staticTest();
String generateFileName();

void setup()
{
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT");

  preferences.begin("app", false);
  loadFactor = preferences.getFloat("loadFactor", -284.6);

  Serial.println("=== BOOT ===");
  printToSerials("loadFactor carregado: " + String(loadFactor, 4));

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT);

  pressureSensor.begin();

  bool sdOk = setupStorageAndFile();
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
  preferences.putFloat("loadFactor", loadFactor);
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

  if (!adc.calibrate(canalCelula))
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

  float peso = abs(rawToKg(raw));
  float pressao = pressureSensor.readMPa();

  if (peso > maxValues[0]) maxValues[0] = peso;
  if (pressao > maxValues[1]) maxValues[1] = pressao;

  leitura = String(ms) + "," + String(peso, 6) + "," + String(pressao, 6);

  printToSerials(leitura);
  appendFile(filedir, leitura);
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

void appendFile(const String &path, const String &message)
{
  File file;

  if (storageType == STORAGE_SD) {
    file = SD.open(path, FILE_APPEND);
  } else if (storageType == STORAGE_LITTLEFS) {
    file = LittleFS.open(path, FILE_APPEND);
  } else {
    Serial.println("ERRO: storage nao inicializado em appendFile");
    return;
  }

  if (!file)
  {
    printToSerials("Falha ao abrir arquivo para append: " + path);
    return;
  }

  bool ok = file.print(message + "\n");
  file.close();

  if (ok)
  {
    digitalWrite(LED_PIN, HIGH);
  }
  else 
  {
    printToSerials("Falha ao gravar linha no arquivo");
    digitalWrite(LED_PIN, LOW);
  }
}

void printToSerials(const String &message)
{
  Serial.println(message);
  SerialBT.println(message);
}

void staticTest()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= INTERVALO)
  {
    previousMillis = currentMillis;
    logData(currentMillis);
  }
}
