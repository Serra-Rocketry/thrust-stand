# thrust-stand

![Status](https://img.shields.io/badge/status-em%20desenvolvimento-yellow)
![Versão](https://img.shields.io/badge/versão-1.0.0-blue)

## 📋 Sobre

Sistema embarcado baseado em ESP32 para monitoramento de testes estáticos de motores de foguetes, com aquisição de empuxo, pressão e temperatura em tempo real, armazenamento em cartão SD e comunicação via Serial/Bluetooth.

## 🚀 Quick Start

1. Clone o repositório

```bash
git clone https://github.com/Serra-Rocketry/thrust-stand.git
```

2. Monte o hardware conforme o [mapeamento de conexões](./docs/HARDWARE.md)
3. Instale a biblioteca `Pushbutton` (Pololu) na Arduino IDE e carregue `firmware/firmware.ino` no ESP32 (placa `ESP32 Dev Module`)
4. Calibre a célula de carga com o [app de calibração](./software/config-load-cell/README.md)
5. Antes do teste, envie `TARE` e, se quiser aliviar o ESP, `BT OFF` (ver [comandos](./docs/API.md))

## 📁 Estrutura do Projeto

```
├── 📂 docs/              # Documentação técnica detalhada
├── 📂 extras/            # Utilitários extras (ex.: configuração do rádio APC220)
├── 📂 firmware/          # Código do microcontrolador ESP32
├── 📂 hardware/          # Esquemáticos, PCBs e lista de componentes
├── 📂 software/          # Ferramentas de análise e interface
└── 📂 Legacy/            # Arquivos antigos que não são mais usados
```

## 🔧 Pré-requisitos

- Hardware: ESP32 DevKit V1, célula de carga + módulo ADS1256, sensor de pressão (com divisor resistivo), termopar tipo K + módulo MAX31855, módulo microSD (SPI), botão tátil (`GPIO33` para TARE), LED com resistor de limitação, buzzer
- Software: Arduino IDE 2.0+ (core ESP32) e Python 3.10+ para o app de calibração
- Bibliotecas: ver `docs/FIRMWARE.md`

## 📖 Documentação

- [Esquemático e Montagem](./docs/HARDWARE.md)
- [Firmware e Arquitetura](./docs/FIRMWARE.md)
- [Protocolos e API](./docs/API.md)
- [Calibração da Célula de Carga](./software/config-load-cell/README.md)
- [Troubleshooting](./docs/TROUBLESHOOTING.md)
- [Evolução da Estrutura (V1 a V3)](./docs/EVOLUÇÃO_CAIXA.md)
- [Detalhes da V3](./docs/V3_DETALHES.md)
- [Arquivos Legados](./Legacy/README.md)

## 🎯 Funcionalidades

- ✅ Aquisição de empuxo e pressão a 500 Hz com ADC de 24 bits (ADS1256)
- ✅ Temperatura com termopar tipo K (MAX31855)
- ✅ Armazenamento em cartão SD com arquivos sequenciais, sem perda de amostras (gravação em núcleo separado)
- ✅ Controle local por botão (TARE)
- ✅ LED de status de gravação no SD
- ✅ Comunicação (Serial, Bluetooth), com `BT OFF` para pausar a transmissão durante o teste
- ✅ Calibração persistente da célula de carga

## 📊 Dados de Teste

Os dados de testes estáticos de motores específicos são armazenados no repositório dedicado:
[motor](https://github.com/Serra-Rocketry/motor)

Este repositório (thrust-stand) contém apenas o sistema de aquisição. Os dados coletados durante os testes devem ser commitados no repositório `motor`.

## 🤝 Contribuindo

Este projeto segue as [Boas Práticas da Serra Rocketry](https://github.com/Serra-Rocketry/best-practices). Para contribuir:

1. Fork o projeto
2. Crie uma branch para sua feature (`git checkout -b feature/AmazingFeature`)
3. Commit suas mudanças (`git commit -m 'Add some AmazingFeature'`)
4. Push para a branch (`git push origin feature/AmazingFeature`)
5. Abra um Pull Request

## ✨ Autores

- Equipe Serra Rocketry - Desenvolvimento e documentação
- Contribuidores - [Lista de contribuidores](https://github.com/Serra-Rocketry/thrust-stand/graphs/contributors)
