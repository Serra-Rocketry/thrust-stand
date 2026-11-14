# 📡 API e Protocolos - Sistema de Teste Estático

## 📋 Visão Geral

Este documento descreve os protocolos de comunicação e as interfaces de controle do sistema de teste estático.

## 🔌 Protocolos Suportados

| Protocolo  | Tipo     | Velocidade  | Alcance | Caso de Uso                    |
| ---------- | -------- | ----------- | ------- | ------------------------------ |
| Serial USB | Fio      | 115200 baud | Local   | Configuração, calibração, debug |
| Bluetooth  | Wireless | 115200 baud | ~10 m   | Telemetria, comandos, monitoramento |

## 💻 Comunicação Serial/Bluetooth

### Configuração padrão

```
BAUD_RATE = 115200
DATA_BITS = 8
PARITY = NONE
STOP_BITS = 1
FLOW_CONTROL = NONE
```

### Estrutura de comandos

```
COMANDO [PARAMETRO]
```

## 🎮 Comandos Disponíveis

### 1. Modo de configuração (calibração)

```
COMANDO: INIT CONFIG
RESPOSTA: Modo calibracao ativo. Aguardando SET LOAD FACTOR...
USO: faz a tara e entra no modo de calibração da célula de carga
     (a célula precisa estar SEM peso: a tara é feita neste momento)

COMANDO: SET LOAD FACTOR <valor>
EXEMPLO: SET LOAD FACTOR 2147.5
RESPOSTA: Fator de carga atualizado
USO: salva o fator de calibração (contagens do ADC por kg) na memória
     persistente e sai do modo de calibração. Só aceito no modo de calibração.
```

### 2. Controle da célula de carga

```
COMANDO: TARE
RESPOSTA: Célula Zerada!
USO: zera a leitura atual da célula de carga

COMANDO: GET LOAD FACTOR
RESPOSTA: loadFactor atual: <valor>
USO: consulta o fator de calibração atual sem entrar em modo de configuração
```

### 3. Transmissão por Bluetooth

```
COMANDO: BT OFF
RESPOSTA: Transmissao BT desligada. Enviando 1 resumo por segundo. Use BT ON para religar.
USO: para de enviar as linhas de dados pelo Bluetooth, para o ESP focar em
     adquirir e gravar. A cada segundo o BT recebe um resumo:
     Gravando t=12s | 35.20 kg (pico 41.87) | 2.31 MPa | 87.5 C
     O BT continua aceitando comandos; a Serial USB e a gravação no SD não mudam.

COMANDO: BT ON
RESPOSTA: Transmissao BT ligada.
USO: volta a enviar as linhas de dados pelo Bluetooth (padrão ao ligar o ESP)
```

## 📈 Linhas de Dados

Fora do modo de calibração, o ESP envia 50 linhas por segundo (1 a cada 10 amostras; o cartão SD recebe todas as 500 por segundo):

```
<tempo_ms>,<empuxo_kg>,<pressao_MPa>,<temperatura_C>
1520,35.201234,2.310000,87.50
```

A temperatura vem como `nan` se o termopar estiver ausente ou com falha.

## 🕹️ Controles Físicos

| Entrada física | Pino ESP32 | Função |
| -------------- | ---------- | ------ |
| Botão TARE | `GPIO33` | TARE da célula de carga |
| LED | `GPIO19` | Indicador de funcionamento (aceso ao gravar no SD) |
| Buzzer | `GPIO32` | Sinalização sonora |

## 🎯 Exemplo de Interação

### Calibração da célula de carga

```
# (célula SEM peso)
# Terminal -> ESP32
INIT CONFIG

# ESP32 -> Terminal
Modo calibracao ativo. Aguardando SET LOAD FACTOR...
# (ESP32 envia a leitura bruta menos a tara a cada 200 ms)
3
-2
1

# (coloca-se um peso de 10 kg)
21471
21478
21469

# fator = média / peso = 21473 / 10 = 2147.3
# Terminal -> ESP32
SET LOAD FACTOR 2147.3

# ESP32 -> Terminal
Fator de carga atualizado: 2147.3000
Modo calibracao finalizado.
```

O app de calibração (`software/config-load-cell/fator_de_calibracao/`) faz esse processo automaticamente.
