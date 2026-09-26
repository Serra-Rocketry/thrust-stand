# ⚙️ Firmware - Sistema de Teste Estático

## 🏗️ Arquitetura do Sistema

### Visão Geral

O firmware implementa aquisição contínua de dados para testes estáticos, com leitura da célula de carga e do sensor de pressão, gravação em cartão SD e transmissão de logs via Serial/Bluetooth.

### Diagrama de Arquitetura

```
┌─────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│   SENSORES      │    │    PROCESSAMENTO │    │     SAÍDAS       │
│                 │    │                  │    │                  │
│  Célula Carga   │───▶│   Aquisição      │───▶│  Cartão SD       │
│  Sensor Pressão │    │   Filtragem      │    │  Serial USB      │
│                 │    │   Formatação CSV │    │  Bluetooth       │
└─────────────────┘    └──────────────────┘    └──────────────────┘
         │                        │                        │
         └────────────────────────┼────────────────────────┘
                                  │
                         ┌──────────────────┐
                         │   CONTROLE       │
                         │                  │
                         │  Botão TARE      │
                         │  Novo arquivo    │
                         │  Estado do LED   │
                         └──────────────────┘
```

## 📁 Estrutura de Código

### Módulos Principais

```
firmware/
├── 📄 firmware.ino              # Arquivo principal
└── 📄 Pressure.h                # Classe sensor de pressão
```

### Dependências e Bibliotecas

- Wire
- HX711
- FS
- SD
- LittleFS
- SPI
- Pushbutton
- BluetoothSerial
- Preferences

Obs.: instale as bibliotecas acima no ambiente Arduino/PlatformIO.

## 🔧 Configuração e Calibração

### Preferências Persistentes

O sistema usa `Preferences` para salvar o fator de calibração da célula de carga.

```cpp
// Carregar configuração
loadFactor = preferences.getFloat("loadFactor", -284.6);

// Salvar configuração
preferences.putFloat("loadFactor", loadFactor);
```

## 🔄 Fluxo de Execução

1. Inicialização (`setup()`)

```cpp
void setup() {
    Serial.begin(115200);
    SerialBT.begin("ESP32_BT");

    preferences.begin("app", false);
    loadFactor = preferences.getFloat("loadFactor", -284.6);

    pressureSensor.begin();

    setupStorageAndFile();
    setupHX711();
}
```

2. Loop principal (`loop()`)

```cpp
void loop() {
    staticTest();  // coleta e log periódico

    // Botão 1: TARE da célula de carga
    // Botão 2: iniciar novo arquivo de log

    if (Serial.available()) {
        // comandos de calibração e controle
    }
}
```

## 📊 Aquisição de Dados

### Estrutura registrada

```csv
Tempo,Empuxo,Pressao
0,0.000000,0.000000
100,1.234567,1.902000
200,2.345678,1.955000
```

### Processo de log

- Leitura da célula de carga (`HX711`)
- Leitura do sensor de pressão (`PressureSensor`)
- Atualização de picos de leitura
- Escrita no arquivo CSV atual no SD/LittleFS
- Espelhamento da leitura em Serial/Bluetooth

## 🎮 Controle do Sistema

### Interface Serial/Bluetooth

Os comandos podem ser enviados pela Serial USB ou pela Bluetooth Serial. Ambas as interfaces recebem e respondem comandos.

### Comandos disponíveis

| Comando         | Descrição                | Exemplo                    | Resposta esperada        |
| --------------- | ------------------------ | -------------------------- | ------------------------ |
| INIT CONFIG     | Entra em modo calibração | `INIT CONFIG`              | Aguarda fator de carga   |
| SET LOAD FACTOR | Define fator de carga    | `SET LOAD FACTOR -284.6`   | Fator atualizado         |
| GET LOAD FACTOR | Consulta fator atual     | `GET LOAD FACTOR`          | loadFactor atual: valor  |
| TARE            | Zera célula de carga     | `TARE`                     | Célula zerada            |

### Entradas físicas

- Botão em `GPIO33`: executa TARE da célula de carga
- LED em `GPIO19`: aceso durante gravação no cartão SD
- Buzzer em `GPIO32`: sinalização sonora

## 💾 Sistema de Arquivos

### Estratégia de armazenamento

- O firmware tenta inicializar o cartão SD
- Em falha do SD, usa LittleFS como fallback
- Arquivos são gerados em sequência (`Dados_001.txt`, `Dados_002.txt`, ...)

### Formato do arquivo de dados

```csv
Tempo,Empuxo,Pressao
0,0.000000,0.000000
100,1.234567,1.902000
```

## 📡 Comunicação

- Serial USB: configuração, calibração e debug
- Bluetooth: espelhamento de leituras/estado

## 🚀 Compilação e Upload

### Arduino IDE

1. Selecione a placa `ESP32 Dev Module`
2. Configure a porta serial correta
3. Verifique as bibliotecas instaladas
4. Compile e faça upload
