#include "HX711.h"
#include "BluetoothSerial.h"

const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 27;

HX711 scale;
BluetoothSerial SerialBT;
float calibration_factor = 100.0;

void printBoth(String msg) {
  Serial.println(msg);
  SerialBT.println(msg);
}

void processCommand(char cmd) {
  switch (cmd) {
    case 'A': calibration_factor += 100.0; scale.set_scale(calibration_factor); break;
    case 'Z': calibration_factor -= 100.0; scale.set_scale(calibration_factor); break;
    case 'a': calibration_factor += 10.0;  scale.set_scale(calibration_factor); break;
    case 'z': calibration_factor -= 10.0;  scale.set_scale(calibration_factor); break;
    case 's': calibration_factor += 1.0;   scale.set_scale(calibration_factor); break;
    case 'x': calibration_factor -= 1.0;   scale.set_scale(calibration_factor); break;
    case 'd': calibration_factor += 0.1;   scale.set_scale(calibration_factor); break;
    case 'c': calibration_factor -= 0.1;   scale.set_scale(calibration_factor); break;
    case 'T':
      printBoth("Retire o peso e aguarde...");
      delay(2000);
      scale.tare();
      printBoth("Tara concluida.");
      break;
    case 'P':
      printBoth("============================");
      printBoth("calibration_factor = " + String(calibration_factor, 4) + ";");
      printBoth("============================");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_Calibracao");
  delay(1000);

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Aguarda HX711 ficar pronto com timeout de 5 segundos
  printBoth("Aguardando HX711...");
  unsigned long timeout = millis();
  while (!scale.is_ready()) {
    if (millis() - timeout > 5000) {
      printBoth("ERRO: HX711 nao respondeu! Verifique a fiacao (DT=26, SCK=27) e a alimentacao.");
      // Fica em loop imprimindo o erro — nao trava silenciosamente
      while (true) {
        Serial.println("HX711 sem resposta. Verifique conexoes e reinicie.");
        delay(2000);
      }
    }
    delay(10);
  }

  printBoth("HX711 detectado OK.");
  scale.set_scale(calibration_factor);
  scale.tare();

  printBoth("Tara OK. Coloque o peso de referencia.");
  printBoth("[A]/[Z] -> +/- 100");
  printBoth("[a]/[z] -> +/- 10");
  printBoth("[s]/[x] -> +/- 1");
  printBoth("[d]/[c] -> +/- 0.1");
  printBoth("[T]     -> Re-tara");
  printBoth("[P]     -> Imprime fator final");
}

void loop() {
  // Verifica se o HX711 continua respondendo
  if (!scale.is_ready()) {
    Serial.println("AVISO: HX711 nao esta pronto, aguardando...");
    delay(200);
    return;
  }

  float peso = abs(scale.get_units());

  String leitura = "Peso: " + String(peso, 1) + " g  |  Fator: " + String(calibration_factor, 2);
  Serial.println(leitura);
  SerialBT.println(leitura);

  while (Serial.available()) {
    processCommand((char)Serial.read());
  }

  while (SerialBT.available()) {
    processCommand((char)SerialBT.read());
  }

  delay(200);
}
