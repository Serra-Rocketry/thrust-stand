#ifndef ADS1256_H
#define ADS1256_H

#include <Arduino.h>
#include <SPI.h>

// Driver mínimo do ADC ADS1256 (Texas Instruments), 24 bits, via SPI.
// Referência: datasheet SBAS288 (ADS1255/ADS1256).
//
// Cada canal (ADS1256Channel) tem o próprio ganho (PGA) e a própria
// calibração interna (OFC/FSC). A cada leitura, os registradores do canal
// são regravados, a conversão é reiniciada (SYNC/WAKEUP) e o driver espera
// o DRDY com o resultado já estabilizado.

// Comandos (datasheet, tabela 24)
#define ADS1256_CMD_WAKEUP  0x00
#define ADS1256_CMD_RDATA   0x01
#define ADS1256_CMD_SDATAC  0x0F
#define ADS1256_CMD_RREG    0x10
#define ADS1256_CMD_WREG    0x50
#define ADS1256_CMD_SELFCAL 0xF0
#define ADS1256_CMD_SYNC    0xFC
#define ADS1256_CMD_RESET   0xFE

// Registradores
#define ADS1256_REG_STATUS 0x00
#define ADS1256_REG_MUX    0x01
#define ADS1256_REG_ADCON  0x02
#define ADS1256_REG_DRATE  0x03
#define ADS1256_REG_IO     0x04
#define ADS1256_REG_OFC0   0x05

// Bits do registrador STATUS
#define ADS1256_STATUS_BUFEN 0x02

// ID de fábrica do ADS1256 (4 bits mais altos do STATUS)
#define ADS1256_CHIP_ID 3

// Entradas do multiplexador
#define ADS1256_AIN0   0
#define ADS1256_AIN1   1
#define ADS1256_AIN2   2
#define ADS1256_AIN3   3
#define ADS1256_AIN4   4
#define ADS1256_AIN5   5
#define ADS1256_AIN6   6
#define ADS1256_AIN7   7
#define ADS1256_AINCOM 8

// Ganho do amplificador interno (PGA): ganho = 2^código
#define ADS1256_PGA_1  0
#define ADS1256_PGA_2  1
#define ADS1256_PGA_4  2
#define ADS1256_PGA_8  3
#define ADS1256_PGA_16 4
#define ADS1256_PGA_32 5
#define ADS1256_PGA_64 6

// Taxas de conversão (registrador DRATE), em amostras por segundo
#define ADS1256_DRATE_30000 0xF0
#define ADS1256_DRATE_15000 0xE0
#define ADS1256_DRATE_7500  0xD0
#define ADS1256_DRATE_3750  0xC0
#define ADS1256_DRATE_2000  0xB0
#define ADS1256_DRATE_1000  0xA1
#define ADS1256_DRATE_500   0x92
#define ADS1256_DRATE_100   0x82
#define ADS1256_DRATE_60    0x72
#define ADS1256_DRATE_50    0x63
#define ADS1256_DRATE_30    0x53
#define ADS1256_DRATE_25    0x43
#define ADS1256_DRATE_15    0x33
#define ADS1256_DRATE_10    0x23
#define ADS1256_DRATE_5     0x13
#define ADS1256_DRATE_2_5   0x03

// Configuração de um canal de leitura
struct ADS1256Channel {
    uint8_t positive; // entrada positiva (ADS1256_AINx)
    uint8_t negative; // entrada negativa (ADS1256_AINx ou ADS1256_AINCOM)
    uint8_t pga;      // ganho (ADS1256_PGA_x)
    uint8_t ofc[3];   // calibração de offset (preenchida por calibrate())
    uint8_t fsc[3];   // calibração de ganho (preenchida por calibrate())
};

class ADS1256 {
public:
    ADS1256(SPIClass &spi, uint8_t csPin, uint8_t drdyPin, float vref)
        : _spi(spi), _csPin(csPin), _drdyPin(drdyPin), _vref(vref),
          _drate(ADS1256_DRATE_1000), _io(0xE1),
          _settings(1800000, MSBFIRST, SPI_MODE1) {} // SCLK máx.: fCLKIN/4 = 1,92 MHz

    // Reinicia o chip, confere o ID e configura taxa e buffer de entrada.
    // O barramento SPI precisa ter sido iniciado antes (spi.begin()).
    bool begin(uint8_t drate, bool inputBuffer) {
        pinMode(_csPin, OUTPUT);
        digitalWrite(_csPin, HIGH);
        pinMode(_drdyPin, INPUT);

        sendCommand(ADS1256_CMD_RESET);
        if (!waitDRDY(1000)) return false; // auto-calibração após o reset
        sendCommand(ADS1256_CMD_SDATAC);

        uint8_t status = readRegister(ADS1256_REG_STATUS);
        if ((status >> 4) != ADS1256_CHIP_ID) return false;

        _drate = drate;
        _io = readRegister(ADS1256_REG_IO);
        // ORDER = MSB primeiro, ACAL desligado (a calibração é feita por canal)
        writeRegister(ADS1256_REG_STATUS, inputBuffer ? ADS1256_STATUS_BUFEN : 0x00);
        writeRegister(ADS1256_REG_DRATE, _drate);

        return readRegister(ADS1256_REG_DRATE) == _drate;
    }

    // Faz a autocalibração interna (offset e ganho) com o PGA do canal
    // e guarda o resultado no próprio canal.
    bool calibrate(ADS1256Channel &channel, uint32_t timeoutMs = 1000) {
        uint8_t regs[2] = {muxValue(channel), adconValue(channel)};
        writeRegisters(ADS1256_REG_MUX, regs, 2);

        sendCommand(ADS1256_CMD_SELFCAL);
        if (!waitDRDY(timeoutMs)) return false;

        uint8_t cal[6];
        readRegisters(ADS1256_REG_OFC0, cal, 6);
        memcpy(channel.ofc, cal, 3);
        memcpy(channel.fsc, cal + 3, 3);
        return true;
    }

    // Lê uma conversão do canal. Retorna false se o DRDY não descer a tempo.
    bool read(const ADS1256Channel &channel, int32_t &value, uint32_t timeoutMs = 1000) {
        // MUX, ADCON, DRATE, IO, OFC0..2, FSC0..2 de uma vez só
        uint8_t regs[10] = {
            muxValue(channel), adconValue(channel), _drate, _io,
            channel.ofc[0], channel.ofc[1], channel.ofc[2],
            channel.fsc[0], channel.fsc[1], channel.fsc[2]
        };
        writeRegisters(ADS1256_REG_MUX, regs, 10);

        sendCommand(ADS1256_CMD_SYNC);
        delayMicroseconds(4); // t11: 24 ciclos de fCLKIN
        sendCommand(ADS1256_CMD_WAKEUP);

        if (!waitDRDY(timeoutMs)) return false;
        value = readData();
        return true;
    }

    // Converte uma leitura bruta em volts, de acordo com o PGA do canal
    float toVolts(int32_t raw, uint8_t pga) const {
        return raw * (2.0f * _vref / (float)(1 << pga)) / 8388607.0f;
    }

    uint8_t readChipId() {
        return readRegister(ADS1256_REG_STATUS) >> 4;
    }

private:
    SPIClass &_spi;
    uint8_t _csPin;
    uint8_t _drdyPin;
    float _vref;
    uint8_t _drate;
    uint8_t _io;
    SPISettings _settings;

    uint8_t muxValue(const ADS1256Channel &channel) const {
        return (channel.positive << 4) | (channel.negative & 0x0F);
    }

    uint8_t adconValue(const ADS1256Channel &channel) const {
        // CLKOUT desligado (menos ruído) e detector de sensor desligado
        return channel.pga & 0x07;
    }

    bool waitDRDY(uint32_t timeoutMs) {
        uint32_t start = millis();
        while (digitalRead(_drdyPin) == HIGH) {
            if (millis() - start > timeoutMs) return false;
        }
        return true;
    }

    void sendCommand(uint8_t command) {
        _spi.beginTransaction(_settings);
        digitalWrite(_csPin, LOW);
        _spi.transfer(command);
        digitalWrite(_csPin, HIGH);
        _spi.endTransaction();
    }

    void writeRegisters(uint8_t start, const uint8_t *data, uint8_t count) {
        _spi.beginTransaction(_settings);
        digitalWrite(_csPin, LOW);
        _spi.transfer(ADS1256_CMD_WREG | start);
        _spi.transfer(count - 1);
        for (uint8_t i = 0; i < count; i++) {
            _spi.transfer(data[i]);
        }
        digitalWrite(_csPin, HIGH);
        _spi.endTransaction();
        delayMicroseconds(1); // t11: 4 ciclos de fCLKIN
    }

    void writeRegister(uint8_t reg, uint8_t value) {
        writeRegisters(reg, &value, 1);
    }

    void readRegisters(uint8_t start, uint8_t *data, uint8_t count) {
        _spi.beginTransaction(_settings);
        digitalWrite(_csPin, LOW);
        _spi.transfer(ADS1256_CMD_RREG | start);
        _spi.transfer(count - 1);
        delayMicroseconds(7); // t6: 50 ciclos de fCLKIN
        for (uint8_t i = 0; i < count; i++) {
            data[i] = _spi.transfer(0xFF);
        }
        digitalWrite(_csPin, HIGH);
        _spi.endTransaction();
        delayMicroseconds(1);
    }

    uint8_t readRegister(uint8_t reg) {
        uint8_t value;
        readRegisters(reg, &value, 1);
        return value;
    }

    int32_t readData() {
        _spi.beginTransaction(_settings);
        digitalWrite(_csPin, LOW);
        _spi.transfer(ADS1256_CMD_RDATA);
        delayMicroseconds(7); // t6: 50 ciclos de fCLKIN
        uint32_t value = (uint32_t)_spi.transfer(0xFF) << 16;
        value |= (uint32_t)_spi.transfer(0xFF) << 8;
        value |= _spi.transfer(0xFF);
        digitalWrite(_csPin, HIGH);
        _spi.endTransaction();

        // Estende o sinal de 24 para 32 bits (complemento de dois)
        if (value & 0x800000) value |= 0xFF000000;
        return (int32_t)value;
    }
};

#endif
