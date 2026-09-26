# thrust-stand

![Status](https://img.shields.io/badge/status-em%20desenvolvimento-yellow)
![Versão](https://img.shields.io/badge/versão-1.0.0-blue)

## 📋 Sobre

Sistema embarcado baseado em ESP32 para monitoramento de testes estáticos de motores de foguetes, com aquisição de dados de empuxo e pressão em tempo real, armazenamento em cartão SD e comunicação via Serial/Bluetooth.

## 🚀 Quick Start

1. Clone o repositório

```bash
git clone https://github.com/ViniciusCMB/thrust-stand.git
```

2. Configure o hardware conforme [esquemático](./docs/HARDWARE.md)
3. Carregue o firmware

## 📁 Estrutura do Projeto

```
├── 📂 docs/              # Documentação técnica detalhada
├── 📂 extras/            # Códigos extras utilizados
├── 📂 firmware/          # Código do microcontrolador ESP32
├── 📂 hardware/          # Esquemáticos, PCBs e lista de componentes
├── 📂 software/          # Ferramentas de análise e interface
└── 📂 tests/             # Testes unitários e de integração
```

## 🔧 Pré-requisitos

- Hardware: ESP32 DevKit V1, célula de carga + HX711, sensor de pressão (com divisor resistivo), módulo microSD (SPI), botão tátil (`GPIO33` para TARE), LED com resistor de limitação, buzzer
- Software: Arduino IDE 2.0+
- Bibliotecas: ver `docs/FIRMWARE.md`

## 📖 Documentação

- [Esquemático e Montagem](./docs/HARDWARE.md)
- [Firmware e Arquitetura](./docs/FIRMWARE.md)
- [Protocolos e API](./docs/API.md)
- [Troubleshooting](./docs/TROUBLESHOOTING.md)

## 🎯 Funcionalidades

- ✅ Aquisição de dados de empuxo e pressão em tempo real
- ✅ Armazenamento em cartão SD com arquivos sequenciais
- ✅ Controle local por botões (TARE e novo arquivo)
- ✅ LED de status de gravação no SD
- ✅ Comunicação (Serial, Bluetooth)
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
- Contribuidores - [Lista de contribuidores]()
