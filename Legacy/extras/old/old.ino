// INCLUSÃO DE BIBLIOTECAS
#include <Wire.h>
#include "RTClib.h"
#include "HX711.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Pushbutton.h>
#include "BluetoothSerial.h" 

// Definições de pinos e constantes
#define RELE_PIN 25       // Pino de controle do relé
#define BTN_PIN 33        // Pino do botão
#define LED_PIN 4         // Pino do LED
#define CS_PIN 5          // Pino do cartão SD
#define CELULA_DT_PIN 26  // Pino de dados da célula de carga
#define CELULA_SCK_PIN 27 // Pino de clock da célula de carga
#define SELECT_PIN 32     // Pino de seleção de modo
#define INTERVALO 10      // Precisão Leitura Dados milissegundos

// Instanciação de objetos
Pushbutton button(BTN_PIN); // Botão
RTC_DS3231 rtc;             // Relógio
HX711 escala;               // Célula de carga
BluetoothSerial SerialBT;   // Bluetooth

// Variáveis globais
const float fator_calib = 277306; // Valor encontrado na calibração
unsigned long previousMillis = 0; // Controle de tempo
long int cont = 0;                // Contador de leituras
String dir = "";                  // Diretório
String filedir = "";              // Arquivo
char cmd;                         // Comando
bool state = false;               // Estado
bool select_loop = false;         // Modo de operação
String leitura = "";              // Leitura dos dados
String msg = "";                  // Mensagem de leitura

void setup()
{
  // Inicialização da comunicação serial
  Serial.begin(9600);
  SerialBT.begin("ESP32_BT"); // Inicialização do Bluetooth com nome "ESP32_BT"

  // Configuração dos pinos
  pinMode(BTN_PIN, INPUT);
  pinMode(SELECT_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, HIGH);

  // Definição do modo de operação
  select_loop = digitalRead(SELECT_PIN) == HIGH; // Seleção de modo, HIGH = Ignitor + Teste Estático, LOW = Apenas ignitor
  if (select_loop)
  {
    // Configuração dos módulos
    setupRTC();
    setupSDCard();
    setupHX711();
    printToSerials("Ignitor e Teste Estático");
  }
  else
  {
    printToSerials("Ignitor");
  }
}

void loop()
{
  // Seleção do modo de operação
  if (select_loop)
  {
    ign_estatico();
  }
  else
  {
    ign();
  }
}

void setupRTC()
{
  // Inicialização do RTC
  if (!rtc.begin())
  {
    while (1)
    {
      printToSerials("DS3231 não encontrado");
    }
  }
  if (rtc.lostPower())
  {
    printToSerials("DS3231 OK!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Ajuste automático
  }
  delay(100);
}

void setupSDCard()
{
  // Inicialização do cartão SD
  if (!SD.begin(CS_PIN))
  {
    while (1)
    {
      printToSerials("Falha no SD");
    }
  }
  if (SD.cardType() == CARD_NONE)
  {
    while (1)
    {
      printToSerials("Cartão SD não encontrado");
    }
  }
  // Criação de diretório e arquivo para armazenamento de dados
  dir = "/" + getCurrentDate();
  createDir(SD, dir);
  filedir = dir + "/" + getCurrentDate() + "_raw.txt";
  writeFile(SD, filedir, "\n");
}

void setupHX711()
{
  // Inicialização da célula de carga
  escala.begin(CELULA_DT_PIN, CELULA_SCK_PIN);
  escala.set_scale(fator_calib);
  escala.tare();
}

void handleButtonPress()
{
  // Tratamento do pressionamento do botão
  escala.tare();
  dir = "/" + getCurrentDate();
  createDir(SD, dir);
  filedir = dir + "/" + getCurrentDate() + "_raw.txt";
  writeFile(SD, filedir, "\n");
  cont = 0;
}

void logData(unsigned long millisec)
{
  // Registro de dados no cartão SD
  String dataAgora = getCurrentDateTime();
  float peso = escala.get_units();
  cont += millisec;
  msg = String(peso, 3) + ";" + String(cont);
  leitura = dataAgora + ";" + msg;

  appendFile(SD, filedir, leitura);
}

String getCurrentDate()
{

  DateTime now = rtc.now();
  String data = "";

  data.concat(String(now.day(), DEC));
  data.concat('-');
  data.concat(String(now.month(), DEC));
  data.concat('-');
  data.concat(String(now.year(), DEC));
  data.concat(';');
  data.concat(String(now.hour(), DEC));
  data.concat('-');
  data.concat(String(now.minute(), DEC));
  data.concat('-');
  data.concat(String(now.second(), DEC));

  return data;
}

String getCurrentDateTime()
{

  DateTime now = rtc.now();
  String data = "";

  data.concat(String(now.day(), DEC));
  data.concat('/');
  data.concat(String(now.month(), DEC));
  data.concat('/');
  data.concat(String(now.year(), DEC));
  data.concat(';');
  data.concat(String(now.hour(), DEC));
  data.concat(':');
  data.concat(String(now.minute(), DEC));
  data.concat(':');
  data.concat(String(now.second(), DEC));

  return data;
}

void writeFile(fs::FS &fs, String path, String message)
{

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    while (true)
    {
      printToSerials("Falha ao abrir o arquivo");
    }
    return;
  }
  if (file.print(message))
  {
    digitalWrite(LED_PIN, HIGH);
  }
  else
  {
    while (true)
    {
      printToSerials("Falha ao escrever no arquivo - w");
    }
    digitalWrite(LED_PIN, LOW);
  }
  file.close();
}

void appendFile(fs::FS &fs, const String &path, const String &message)
{
  // Adição de dados em um arquivo
  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    while (true)
    {
      printToSerials("Falha ao abrir o arquivo");
    }
    return;
  }
  if (file.print(message + "\n"))
  {
    digitalWrite(LED_PIN, HIGH);
  }
  else
  {
    while (true)
    {
      printToSerials("Falha ao escrever no arquivo - a");
    }
    digitalWrite(LED_PIN, LOW);
  }
  file.close();
}

void createDir(fs::FS &fs, const String &path)
{
  // Criação de diretório
  if (fs.mkdir(path))
  {
    printToSerials("Dir created");
  }
  else
  {
    printToSerials("mkdir failed");
  }
}

void printToSerials(const String &message)
{
  // Função para imprimir na Serial e SerialBT
  Serial.println(message);
  SerialBT.println(message);
}

void ign_estatico()
{
  unsigned long currentMillis = millis();
  unsigned long millisec = currentMillis - previousMillis;
  logData(millisec);
  previousMillis = currentMillis;

  if (Serial.available())
  {
    cmd = Serial.read(); // Leitura do comando recebido via serial (USB)
  }
  else if (SerialBT.available())
  {
    cmd = SerialBT.read(); // Leitura do comando recebido via Bluetooth
  }
  if (cmd != ' ') // Verifica se o comando não está em branco
  {
    switch (cmd)
    {
      case 'A': // Aguardando comando
        printToSerials(msg);
        break;
      case 'C': // Contagem iniciada
        printToSerials("Iniciando contagem.");
        break;
      case 'E': // Tarar célula de carga
        printToSerials("Zerando célula.");
        escala.tare();
        break;
      case 'R': // Reiniciar o esp
        printToSerials("Reiniciando.");
        ESP.restart();
        break;
      case '1': // Rele ativado
        printToSerials("Ativado.");
        digitalWrite(RELE_PIN, LOW);
        break;
      case '0': // Rele desativado
        printToSerials("Desativado.");
        digitalWrite(RELE_PIN, HIGH);
        break;
      default:
        printToSerials("Comando desconhecido.");
        break;
    }
    cmd = ' ';
  }
  if (button.getSingleDebouncedPress())
  {
    handleButtonPress();
  }
}

void ign()
{
  // Função de ignição
  if (Serial.available())
  {
    cmd = Serial.read(); // Leitura do comando recebido via serial (USB)
  }
  else if (SerialBT.available())
  {
    cmd = SerialBT.read(); // Leitura do comando recebido via Bluetooth
  }

  if (cmd != ' ') // Verifica se o comando não está em branco
  {
    switch (cmd)
    {
      case 'A': // Aguardando comando
        printToSerials("Aguardando comando.");
        break;
      case 'C': // Contagem iniciada
        printToSerials("Iniciando contagem.");
        break;
      case 'R': // Reiniciar o esp
        printToSerials("Reiniciando.");
        ESP.restart();
        break;
      case '1': // Rele ativado
        printToSerials("Ativado.");
        digitalWrite(RELE_PIN, LOW);
        break;
      case '0': // Rele desativado
        printToSerials("Desativado.");
        digitalWrite(RELE_PIN, HIGH);
        break;
      default:
        printToSerials("Comando esconhecido.");
        break;
    }
    cmd = ' ';
  }
}