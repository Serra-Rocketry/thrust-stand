#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

class PressureSensor {
public:
    PressureSensor(uint8_t pin, float resistor1, float resistor2, int adcResolution, float adcMaxVoltage,
                   float vOutMin, float vOutMax, float pMaxMPa)
        : _pin(pin), _resistor1(resistor1), _resistor2(resistor2), _adcResolution(adcResolution), _adcMaxVoltage(adcMaxVoltage),
          _vOutMin(vOutMin), _vOutMax(vOutMax), _pMaxMPa(pMaxMPa) {}

    void begin() {
        pinMode(_pin, INPUT);
    }

    float readMPa() {
        int adcValue = analogRead(_pin);
        float voltageAtPin = (adcValue / (float)_adcResolution) * _adcMaxVoltage;
        float sensorVoltage = voltageAtPin * (_resistor1 + _resistor2) / _resistor2;
        float pressureMPa = (sensorVoltage - _vOutMin) * _pMaxMPa / (_vOutMax - _vOutMin);
        if (pressureMPa < 0) pressureMPa = 0;
        return pressureMPa;
    }

    float readBar() {
        return readMPa() * 10.0;
    }

    float readPSI() {
        return readMPa() * 145.038;
    }

    int readADC(){
        int adcValue = analogRead(_pin);
        return adcValue;
    }

private:
    uint8_t _pin;
    float _resistor1, _resistor2;
    int _adcResolution;
    float _adcMaxVoltage;
    float _vOutMin, _vOutMax, _pMaxMPa;
};

#endif