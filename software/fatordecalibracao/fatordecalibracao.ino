// Calibração manual do fator da célula de carga com o ADS1256.
// Coloque um peso conhecido e ajuste o fator pelas teclas até a leitura
// bater com o peso. Funciona pela Serial (115200) e pelo Bluetooth.
//
// Usa a mesma pinagem e a mesma chave de NVS do firmware principal:
// o fator salvo com [S] é carregado pelo firmware/firmware.ino no boot.

#include <SPI.h>
#include <BluetoothSerial.h>
#include <Preferences.h>

#include "ADS1256.h" // cópia de firmware/ADS1256.h (mantenha as duas iguais)

#define ADS_SCLK_PIN 18
#define ADS_DOUT_PIN 4
#define ADS_DIN_PIN 13
#define ADS_CS_PIN 26
#define ADS_DRDY_PIN 34
#define TARE_AMOSTRAS 50
#define MEDIA_AMOSTRAS 10 // leituras por valor mostrado (deixa o número mais estável)
#define NVS_CHAVE_FATOR "fatorADS1256"

const float ADS_VREF = 2.5;
const float FATOR_TEORICO = 2147.48; // ver firmware.ino (SZ51X-500kg, 2 mV/V, 5 V, PGA 64)

SPIClass spiADC(HSPI);
ADS1256 adc(spiADC, ADS_CS_PIN, ADS_DRDY_PIN, ADS_VREF);
ADS1256Channel canalCelula = {ADS1256_AIN0, ADS1256_AIN1, ADS1256_PGA_64};
BluetoothSerial SerialBT;
Preferences preferences;

float calibration_factor = FATOR_TEORICO;
long tareOffset = 0;

void printBoth(String msg)
{
  Serial.println(msg);
  SerialBT.println(msg);
}

bool readAverage(int amostras, long &media)
{
  int64_t soma = 0;
  for (int i = 0; i < amostras; i++)
  {
    int32_t valor;
    if (!adc.read(canalCelula, valor, 1000))
      return false;
    soma += valor;
  }
  media = soma / amostras;
  return true;
}

void tare()
{
  long media;
  if (readAverage(TARE_AMOSTRAS, media))
  {
    tareOffset = media;
    printBoth("Tara concluida.");
  }
  else
  {
    printBoth("ERRO: ADS1256 nao respondeu na tara.");
  }
}

void processCommand(char cmd)
{
  switch (cmd)
  {
    case 'A': calibration_factor += 100.0; break;
    case 'Z': calibration_factor -= 100.0; break;
    case 'a': calibration_factor += 10.0;  break;
    case 'z': calibration_factor -= 10.0;  break;
    case 's': calibration_factor += 1.0;   break;
    case 'x': calibration_factor -= 1.0;   break;
    case 'd': calibration_factor += 0.1;   break;
    case 'c': calibration_factor -= 0.1;   break;
    case 'T':
      printBoth("Retire o peso e aguarde...");
      delay(2000);
      tare();
      break;
    case 'P':
      printBoth("============================");
      printBoth("calibration_factor = " + String(calibration_factor, 4) + ";");
      printBoth("============================");
      break;
    case 'S':
      preferences.putFloat(NVS_CHAVE_FATOR, calibration_factor);
      printBoth("Fator salvo no ESP: " + String(calibration_factor, 4));
      break;
  }
}

void setup()
{
  Serial.begin(115200);
  SerialBT.begin("ESP32_Calibracao");
  delay(1000);

  preferences.begin("app", false);
  calibration_factor = preferences.getFloat(NVS_CHAVE_FATOR, FATOR_TEORICO);

  printBoth("Aguardando ADS1256...");
  spiADC.begin(ADS_SCLK_PIN, ADS_DOUT_PIN, ADS_DIN_PIN, -1);
  delay(100);

  if (!adc.begin(ADS1256_DRATE_2000, true) || !adc.calibrate(canalCelula))
  {
    // Fica em loop imprimindo o erro — nao trava silenciosamente
    while (true)
    {
      printBoth("ERRO: ADS1256 nao respondeu! Verifique SCLK=18, DOUT=4, DIN=13, CS=26, DRDY=34 e a alimentacao.");
      delay(2000);
    }
  }

  printBoth("ADS1256 detectado OK.");
  tare();

  printBoth("Coloque o peso de referencia.");
  printBoth("[A]/[Z] -> +/- 100");
  printBoth("[a]/[z] -> +/- 10");
  printBoth("[s]/[x] -> +/- 1");
  printBoth("[d]/[c] -> +/- 0.1");
  printBoth("[T]     -> Re-tara");
  printBoth("[P]     -> Imprime fator final");
  printBoth("[S]     -> Salva o fator no ESP (usado pelo firmware principal)");
}

void loop()
{
  long media;
  if (!readAverage(MEDIA_AMOSTRAS, media))
  {
    Serial.println("AVISO: ADS1256 nao esta pronto, aguardando...");
    delay(200);
    return;
  }

  float peso = (media - tareOffset) / calibration_factor;
  printBoth("Peso: " + String(peso, 3) + " kg  |  Fator: " + String(calibration_factor, 2));

  while (Serial.available())
  {
    processCommand((char)Serial.read());
  }

  while (SerialBT.available())
  {
    processCommand((char)SerialBT.read());
  }

  delay(200);
}
