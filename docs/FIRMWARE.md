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
├── 📄 ADS1256.h                 # Driver do ADC ADS1256 (célula de carga e pressão)
├── 📄 MAX31855.h                # Driver do termopar tipo K (MAX31855)
└── 📄 Pressure.h                # Conversão tensão -> pressão
```

### Dependências e Bibliotecas

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

O sistema usa `Preferences` para salvar o fator de calibração da célula de carga (contagens do ADC por kg).

```cpp
// Carregar configuração (namespace "app")
loadFactor = preferences.getFloat("fatorADS1256", FATOR_TEORICO);

// Salvar configuração
preferences.putFloat("fatorADS1256", loadFactor);
```

- A chave antiga, `loadFactor`, guardava o fator do HX711 e não é mais lida.
- Enquanto a célula não for calibrada, vale o **fator teórico** (~2147,5), calculado pelo certificado da SZ51X-500kg (2 mV/V, excitação de 5 V, 500 kg) e pelo fundo de escala do ADS1256 no ganho 64.
- O sinal do fator define o sentido do empuxo (não há mais `abs()` na leitura).

## 🔄 Fluxo de Execução

1. Inicialização (`setup()`)

```cpp
void setup() {
    Serial.begin(115200);
    SerialBT.begin("ESP32_BT");

    preferences.begin("app", false);
    loadFactor = preferences.getFloat("fatorADS1256", FATOR_TEORICO);

    setupADS1256();         // reset, autocalibração por canal e tara inicial
    setupTermopar();        // opcional: sem termopar, grava "nan"
    setupStorageAndFile();  // só cria o arquivo se o ADS1256 respondeu
    startRecording();       // abre o arquivo e cria a tarefa de gravação
}
```

Se o ADS1256 ou o armazenamento falharem, o ESP toca o alerta e reinicia.

2. Aquisição e gravação em dois núcleos

```
 Núcleo 1: loop()                          Núcleo 0: recordingTask
 ─────────────────                         ──────────────────────
 a cada 2 ms (500 Hz):                     tira a amostra da fila
   lê célula (ADS1256)                     grava a linha no arquivo
   lê pressão (ADS1256)    ── fila ──▶     atualiza os picos
   temperatura (a cada 125 ms)  (1024)     1 a cada 10 → Serial/BT
   coloca a amostra na fila                flush no cartão a cada 1 s
 comandos e botão TARE
```

- A leitura dos sensores nunca espera o cartão SD: se o cartão demorar, as amostras aguardam na fila (até ~2 s).
- Se a fila encher, as amostras perdidas são contadas e avisadas por Serial/BT.
- O arquivo fica aberto durante todo o teste. Se a energia cair, perde-se no máximo o último segundo.

## 📊 Aquisição de Dados

### Taxas

| Parâmetro | Valor | Constante |
| --------- | ----- | --------- |
| Amostragem (empuxo + pressão) | 500 Hz | `TAXA_AMOSTRAGEM_HZ` |
| Taxa interna do ADS1256 | 2000 SPS | `ADS1256_DRATE_2000` em `setupADS1256()` |
| Termopar | 8 Hz | `TERMOPAR_INTERVALO_MS` (125 ms) |
| Transmissão Serial/BT | 50 linhas/s | `TRANSMISSAO_HZ` |
| Flush no cartão | 1 s | `FLUSH_INTERVALO_MS` |

**Por que 500 Hz:** a subida do empuxo na ignição leva poucas dezenas de milissegundos. A 500 Hz há ~10 pontos numa subida de 20 ms; 1000 Hz dobraria os dados e o ruído sem ganho real. Como o ADS1256 alterna entre célula e pressão, a taxa interna fica mais alta (2000 SPS) para sobrar tempo para os dois canais.

### Processo de log

- Leitura da célula de carga pelo ADS1256 (`AIN0`/`AIN1`, ganho 64)
- Leitura do sensor de pressão pelo ADS1256 (`AIN2`, ganho 1) e conversão com `PressureSensor`
- Leitura do termopar pelo MAX31855 (repetindo o último valor entre leituras)
- Gravação de todas as amostras no arquivo CSV atual (SD ou LittleFS)
- Transmissão de 1 a cada 10 amostras por Serial/Bluetooth

## 🎮 Controle do Sistema

### Interface Serial/Bluetooth

Os comandos podem ser enviados pela Serial USB ou pela Bluetooth Serial. Ambas as interfaces recebem e respondem comandos.

### Comandos disponíveis

| Comando         | Descrição                | Exemplo                    | Resposta esperada        |
| --------------- | ------------------------ | -------------------------- | ------------------------ |
| INIT CONFIG     | Faz a tara e entra em modo calibração (célula **sem peso**) | `INIT CONFIG` | Aguarda fator de carga |
| SET LOAD FACTOR | Define fator de carga (só no modo calibração) | `SET LOAD FACTOR 2147.5` | Fator atualizado |
| GET LOAD FACTOR | Consulta fator atual     | `GET LOAD FACTOR`          | loadFactor atual: valor  |
| TARE            | Zera célula de carga     | `TARE`                     | Célula zerada            |
| BT OFF          | Para de enviar dados pelo BT; manda 1 resumo por segundo | `BT OFF` | Transmissao BT desligada |
| BT ON           | Volta a enviar dados pelo BT | `BT ON`                | Transmissao BT ligada    |

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
Tempo,Empuxo,Pressao,Temperatura
1520,0.012345,0.101234,24.75
1522,0.010987,0.101198,24.75
1524,0.013210,0.101305,24.75
```

| Coluna | Unidade | Observação |
| ------ | ------- | ---------- |
| Tempo | ms desde o boot | uma linha a cada 2 ms (500 Hz) |
| Empuxo | kg | negativo se o fator estiver com o sinal invertido |
| Pressao | MPa | |
| Temperatura | °C | `nan` se o termopar estiver ausente ou com falha |

## 📡 Comunicação

- Serial USB: configuração, calibração, debug e 50 linhas de dados por segundo
- Bluetooth: os mesmos dados e comandos; com `BT OFF`, só um resumo por segundo:

```
Gravando t=12s | 35.20 kg (pico 41.87) | 2.31 MPa | 87.5 C
```

## 🚀 Compilação e Upload

### Arduino IDE

1. Selecione a placa `ESP32 Dev Module`
2. Configure a porta serial correta
3. Verifique as bibliotecas instaladas (só a `Pushbutton`, da Pololu, é externa)
4. Compile e faça upload

### Arduino CLI

```bash
arduino-cli lib install Pushbutton
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/
```
