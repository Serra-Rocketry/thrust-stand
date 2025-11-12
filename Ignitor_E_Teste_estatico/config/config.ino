#include <SoftwareSerial.h>

const int pinoRX = 6;
const int pinoTX = 7;
const int pinoSET = 5;

SoftwareSerial apc220(pinoRX, pinoTX);

void setSettings(void)
{
    // Coloca o pino SET em low e habilita o modo config do APC220
    digitalWrite(pinoSET, LOW);
    delay(10);
    // Escreve os parametro no modulo
    apc220.println("WR 435000 3 9 3 0");
    delay(10);
}

void getSettings(void)
{
    // Rotina de leitura dos parametros
    digitalWrite(pinoSET, LOW);
    delay(10);
    apc220.println("RD");
    delay(10);
    while (apc220.available())
    {
        Serial.write(apc220.read());
    }
    digitalWrite(pinoSET, HIGH);
    delay(200);
}

void setup()
{
    Serial.begin(9600);
    pinMode(pinoSET, HIGH);
    apc220.begin(9600);
}

void loop()
{
    Serial.println("Gravando as configuracoes no modulo APC220...");
    setSettings();
    delay(1000);
    Serial.println("nConfiguracao finalizada. Lendo os parametros...");
    getSettings();
    Serial.println("nModulo configurado!");
    delay(100000);
}