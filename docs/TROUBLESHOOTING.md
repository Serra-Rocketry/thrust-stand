# 🐛 Troubleshooting - Sistema de Teste Estático

## 📋 Visão Geral

Guia rápido para diagnosticar falhas de alimentação, gravação no SD, leitura de sensores e controle por botões.

## 🔍 Diagnóstico Rápido

### LED de funcionamento (`GPIO19`)

| Estado do LED | Significado | Ação |
| ------------- | ----------- | ---- |
| Aceso | Sistema gravando no SD | Operação normal |
| Apagado durante gravação esperada | Sem escrita no arquivo atual | Verificar SD e fluxo de gravação |

### Botão TARE

| Botão | Pino | Função esperada |
| ----- | ---- | --------------- |
| Botão TARE | `GPIO33` | TARE da célula de carga |

## 🚨 Problemas Comuns e Soluções

### 1. 🔌 Problemas de alimentação

Sintomas:

- Sistema não liga
- Reinicializações aleatórias
- Leituras instáveis

Soluções:

```
1. Medir 5V em VIN e 3.3V no ESP32
2. Confirmar fonte com corrente suficiente (>= 2A recomendado)
3. Testar com outro cabo USB/fonte
4. Confirmar GND comum entre todos os módulos
```

### 2. 💾 Problemas com cartão SD

Sintomas:

- Falha ao iniciar armazenamento
- Arquivo não é criado
- LED não acende quando deveria gravar

Soluções:

```
1. Verificar formatação FAT32
2. Verificar conexões SPI: CS(5), MOSI(27), MISO(23), SCK(25)
3. Testar outro cartão SD
4. Verificar alimentação e GND do módulo SD
```

### 3. ⚖️ Problemas com célula de carga / HX711

Sintomas:

- Leitura travada em valor fixo
- Leitura muito ruidosa
- TARE não zera

Soluções:

```
1. Verificar DT(GPIO4) e SCK(GPIO18)
2. Verificar ligação da célula no HX711
3. Executar TARE pelo botão GPIO33 e por comando serial (TARE)
4. Repetir calibração com peso conhecido
```

### 4. 📊 Problemas com sensor de pressão

Sintomas:

- Leitura em 0 constante
- Saturação em valor máximo
- Oscilação elevada

Soluções:

```
1. Verificar alimentação do sensor
2. Verificar divisor resistivo (2.2kΩ e 3.3kΩ)
3. Medir tensão no GPIO35 (faixa esperada: 0-3.3V)
4. Conferir GND comum entre sensor e ESP32
```

### 5. 🎛️ Problemas nos botões

Sintomas:

- Botão de TARE não responde

Soluções:

```
1. Conferir ligação dos botões para GND
2. Conferir pino correto: GPIO33 (TARE)
3. Verificar mau contato no botão/chicote
4. Testar acionamento com monitor serial aberto
```

### 6. 📡 Problemas de comunicação Serial/Bluetooth

Sintomas:

- Sem dados no terminal serial
- Bluetooth não encontra o dispositivo

Soluções:

```
1. Confirmar baud rate 115200
2. Testar outro cabo USB
3. Confirmar porta serial correta
4. Reiniciar ESP32 e reconectar Bluetooth
```
