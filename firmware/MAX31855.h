#ifndef MAX31855_H
#define MAX31855_H

#include <Arduino.h>
#include <SPI.h>

// Driver mínimo do MAX31855K (Maxim/Analog Devices): termopar tipo K,
// com compensação de junta fria embutida. Só leitura, via SPI (modo 0).
//
// O chip converte continuamente (70 a 100 ms por conversão) e baixar o
// CS interrompe a conversão em andamento: leia no máximo a cada ~100 ms,
// senão o valor nunca é atualizado.

// Bits de falha (3 bits mais baixos da leitura)
#define MAX31855_FALHA_ABERTO    0x01 // termopar desconectado (circuito aberto)
#define MAX31855_FALHA_CURTO_GND 0x02 // termopar em curto com GND
#define MAX31855_FALHA_CURTO_VCC 0x04 // termopar em curto com VCC
#define MAX31855_SEM_RESPOSTA    0x80 // nenhum chip respondeu no barramento

class MAX31855 {
public:
    MAX31855(SPIClass &spi, uint8_t csPin)
        : _spi(spi), _csPin(csPin), _fault(0),
          _settings(4000000, MSBFIRST, SPI_MODE0) {} // SCK máx.: 5 MHz

    void begin() {
        pinMode(_csPin, OUTPUT);
        digitalWrite(_csPin, HIGH);
    }

    // Temperatura na ponta do termopar, em °C (resolução de 0,25 °C).
    // Retorna false em caso de falha; veja lastFault().
    bool readCelsius(float &temperatura) {
        uint32_t v = readRaw();

        if (v == 0x00000000 || v == 0xFFFFFFFF) {
            _fault = MAX31855_SEM_RESPOSTA;
            return false;
        }
        if (v & 0x00010000) { // bit de falha
            _fault = v & 0x07;
            return false;
        }

        _fault = 0;
        int32_t raw = v >> 18; // 14 bits com sinal
        if (raw & 0x2000) raw -= 0x4000;
        temperatura = raw * 0.25f;
        _internal = internalFromRaw(v);
        return true;
    }

    // Temperatura da junta fria (dentro do chip) da última leitura válida, em °C
    float internalCelsius() const {
        return _internal;
    }

    // Falha da última leitura (0 = sem falha)
    uint8_t lastFault() const {
        return _fault;
    }

private:
    SPIClass &_spi;
    uint8_t _csPin;
    uint8_t _fault;
    float _internal = NAN;
    SPISettings _settings;

    static float internalFromRaw(uint32_t v) {
        int32_t raw = (v >> 4) & 0x0FFF; // 12 bits com sinal
        if (raw & 0x0800) raw -= 0x1000;
        return raw * 0.0625f;
    }

    uint32_t readRaw() {
        _spi.beginTransaction(_settings);
        digitalWrite(_csPin, LOW);
        delayMicroseconds(1);
        uint32_t v = 0;
        for (int i = 0; i < 4; i++) {
            v = (v << 8) | _spi.transfer(0x00);
        }
        digitalWrite(_csPin, HIGH);
        _spi.endTransaction();
        return v;
    }
};

#endif
