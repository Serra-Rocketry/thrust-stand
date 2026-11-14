#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

// Converte a tensão lida depois do divisor resistivo em pressão.
// A leitura da tensão em si é feita pelo ADS1256.
class PressureSensor {
public:
    PressureSensor(float resistor1, float resistor2, float vOutMin, float vOutMax, float pMaxMPa)
        : _resistor1(resistor1), _resistor2(resistor2),
          _vOutMin(vOutMin), _vOutMax(vOutMax), _pMaxMPa(pMaxMPa) {}

    // voltageAtPin: tensão medida no ponto do divisor (entre R1 e R2)
    float toMPa(float voltageAtPin) const {
        float sensorVoltage = voltageAtPin * (_resistor1 + _resistor2) / _resistor2;
        float pressureMPa = (sensorVoltage - _vOutMin) * _pMaxMPa / (_vOutMax - _vOutMin);
        if (pressureMPa < 0) pressureMPa = 0;
        return pressureMPa;
    }

    float toBar(float voltageAtPin) const {
        return toMPa(voltageAtPin) * 10.0;
    }

    float toPSI(float voltageAtPin) const {
        return toMPa(voltageAtPin) * 145.038;
    }

private:
    float _resistor1, _resistor2;
    float _vOutMin, _vOutMax, _pMaxMPa;
};

#endif
