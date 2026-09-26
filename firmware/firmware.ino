// INCLUSÃO DE BIBLIOTECAS
#include <HX711.h>
#include <FS.h>
#include <SD.h>
#include <LittleFS.h>
#include <SPI.h>
#include <Pushbutton.h>
#include <BluetoothSerial.h>
#include <Preferences.h>

#include "Pressure.h"

// Definições de pinos e constantes
#define LED_PIN 19        // Pino do LED
#define CS_PIN 5          // Pino do cartão SD (CS)
#define SD_SCK_PIN 25     // Pino SCK do SD
#define SD_MISO_PIN 23    // Pino MISO do SD
#define SD_MOSI_PIN 27    // Pino MOSI do SD
#define BUZZER_PIN 32     // Pino do buzzer
#define BTN_PIN 33        // Pino do botão
#define CELULA_DT_PIN 4   // Pino de dados da célula de carga (DAT)
#define CELULA_SCK_PIN 18 // Pino de clock da célula de carga (CLK)
#define PRESSURE_PIN 35   // Pino do sensor de pressão
#define INTERVALO 1       // Intervalo de leitura em milissegundos

// Instância SPI customizada para o SD (SCK=25, MISO=23, MOSI=27, CS=5)
SPIClass spiSD(VSPI);

// Variáveis globais
const float VinPressure = 5.0;    // Tensão que alimenta o sensor
const float VminPressure = 0.27;  // Tensão de saída em 0 MPa
const float VmaxPressure = 4.5;   // Tensão de saída em 10 MPa
const float maxPressure = 10.0;   // Pressão máxima do sensor em MPa
const float R1 = 2200.0;          // Resistor entre sensor e pino ESP32
const float R2 = 3300.0;          // Resistor entre pino ESP32 e GND
const int RESOLUCAO_ADC = 4095;   // ESP32 tem ADC de 12 bits (2^12 - 1)
const float TENSAO_MAX_ADC = 3.3; // Tensão de referência do ADC do ESP32
float maxValues[2] = {0.0, 0.0};  // Leituras de pico (peso, pressão)
unsigned long previousMillis = 0; // Controle de tempo
float loadFactor = 0.0;           // Fator de calibração
String filedir = "";              // Caminho do arquivo
String leitura = "";              // Leitura dos dados

enum StorageType { STORAGE_NONE, STORAGE_SD, STORAGE_LITTLEFS };
StorageType storageType = STORAGE_NONE;

static bool configMode = false;

// Instanciação de objetos
Pushbutton button(BTN_PIN);
PressureSensor pressureSensor(PRESSURE_PIN, R1, R2, RESOLUCAO_ADC, TENSAO_MAX_ADC, VminPressure, VmaxPressure, maxPressure);
HX711 escala;
BluetoothSerial SerialBT;
Preferences preferences;

// Declarações antecipadas
void processCommand(String command);
void setLoadFactor(float factor);
void buzzSignal(String signal);
bool setupStorage();
bool setupStorageAndFile();
bool setupHX711();
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
  bool hx711Ok = setupHX711();

  if (sdOk && hx711Ok)
  {
    printToSerials("Sistema configurado. Transmitindo...");
    buzzSignal("Sucesso");
  }
  else
  {
    if (!sdOk)   printToSerials("ERRO: Falha no storage.");
    if (!hx711Ok) printToSerials("ERRO: Falha no HX711.");
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
    if (escala.wait_ready_timeout(1000))
    {
      buzzSignal("Beep");
      printToSerials("Celula Zerada!");
      escala.tare();
    }
    else
    {
      printToSerials("HX711 nao pronto, TARE ignorado.");
    }
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
    if (escala.wait_ready_timeout(1000))
    {
      long raw = escala.get_value(1);
      Serial.println(raw);
      SerialBT.println(raw);
    }
    delay(200);
  }
}

void processCommand(String command)
{
  if (command.equalsIgnoreCase("TARE"))
  {
    if (escala.wait_ready_timeout(1000))
    {
      buzzSignal("Beep");
      printToSerials("Celula Zerada!");
      escala.tare();
    }
    else
    {
      printToSerials("HX711 nao pronto, TARE ignorado.");
    }
    return;
  }

  if (command.equalsIgnoreCase("GET LOAD FACTOR"))
  {
    printToSerials("loadFactor atual: " + String(loadFactor, 4));
    return;
  }

  if (command.startsWith("INIT CONFIG"))
  {
    if (escala.wait_ready_timeout(1000))
    {
      escala.tare();
      configMode = true;
      printToSerials("Modo calibracao ativo. Aguardando SET LOAD FACTOR...");
    }
    else
    {
      printToSerials("HX711 nao pronto. Verifique conexoes.");
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
  escala.set_scale(loadFactor);
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

bool setupHX711()
{
  Serial.println("Iniciando HX711...");
  escala.begin(CELULA_DT_PIN, CELULA_SCK_PIN);

  unsigned long timeout = millis();
  while (!escala.is_ready())
  {
    if (millis() - timeout > 5000)
    {
      Serial.println("ERRO: HX711 nao respondeu (timeout 5s). Verifique DT=4, SCK=18.");
      return false;
    }
    delay(10);
  }

  Serial.println("HX711 detectado OK.");
  escala.set_scale(loadFactor);
  Serial.print("set_scale com fator: ");
  Serial.println(loadFactor, 4);

  // Warmup: descarta as primeiras leituras para estabilizar
  Serial.println("Aguardando estabilizacao...");
  for (int i = 0; i < 10; i++)
  {
    escala.get_value(1);
    delay(100);
  }

  escala.tare(10); // média de 10 amostras para tara precisa
  Serial.println("tare OK");

  printToSerials("HX711 conectado");
  return true;
}

void logData(unsigned long ms)
{
  if (!escala.wait_ready_timeout(1000))
  {
    Serial.println("AVISO: HX711 timeout em logData, pulando leitura.");
    return;
  }

  float peso = abs(escala.get_units(1)); // 1 amostra por leitura
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
