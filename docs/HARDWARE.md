# 🔌 Hardware - Sistema de Teste Estático

## 📋 Visão Geral do Sistema

Sistema embarcado baseado em ESP32 para aquisição de empuxo e pressão em testes estáticos de motores de foguetes, com gravação local de dados no cartão SD e telemetria via Serial/Bluetooth.

Controles físicos do esquemático atual:

- `GPIO33`: botão para TARE (zerar a célula de carga)
- `GPIO19`: LED de indicador de funcionamento (aceso durante gravação no SD)
- `GPIO32`: buzzer de sinalização

## 🧩 Lista de Componentes (BOM)

| Componente                | Quantidade | Especificações                                |
| ------------------------- | ---------- | --------------------------------------------- |
| ESP32 DevKit V1           | 1          | 30/38 pinos, WiFi/BT                          |
| Célula de carga           | 1          | SZ51X-500kg, 2 mV/V (ponte de Wheatstone)     |
| Módulo ADS1256 (placa azul) | 1        | ADC 24 bits, 8 canais, SPI, referência 2,5 V  |
| Sensor de pressão         | 1          | 0-10 MPa, saída analógica 0.5-4.5V            |
| Módulo microSD            | 1          | Interface SPI                                 |
| LED                       | 1          | 5mm, vermelho                                 |
| Botão tátil (TARE)        | 1          | 6x6mm                                         |
| Buzzer                    | 1          | Ativo, 5V                                     |
| Resistores do divisor     | 2          | 2.2kΩ e 3.3kΩ                                 |
| Resistor do LED           | 1          | 220Ω (valor típico)                           |
| Fonte de alimentação      | 1          | 5V 2A (mínimo recomendado)                    |

## 🔄 Mapeamento de Conexões

| ESP32 | Função no sistema |
| ----- | ----------------- |
| `GPIO18` | ADS1256 `SCLK` |
| `GPIO4`  | ADS1256 `DOUT` (MISO) |
| `GPIO13` | ADS1256 `DIN` (MOSI) |
| `GPIO26` | ADS1256 `CS` |
| `GPIO34` | ADS1256 `DRDY` |
| `GPIO5`  | microSD `CS` |
| `GPIO27` | microSD `MOSI` |
| `GPIO23` | microSD `MISO` |
| `GPIO25` | microSD `SCK` |
| `GPIO19` | LED de gravação |
| `GPIO32` | Buzzer |
| `GPIO33` | Botão TARE |
| `VIN`    | Alimentação 5V dos módulos |
| `GND`    | Terra comum |

Observações de ligação:

- Botão: um terminal no `GPIO33` e o outro no `GND`.
- LED: ligado ao `GPIO19` com resistor em série.
- Buzzer: ligado ao `GPIO32`.
- Sensor de pressão: saída analógica passa por divisor resistivo antes de chegar à entrada `AIN2` do ADS1256.

Ligações no módulo ADS1256:

| Módulo ADS1256 | Ligação |
| -------------- | ------- |
| `AIN0` / `AIN1` | Sinal da célula de carga (`S+` / `S-`) |
| `AIN2` | Sensor de pressão (ponto do divisor resistivo) |
| `AINCOM` | `GND` |
| `PDWN` | `3,3V` (fixo; o módulo azul não tem pino `RESET`, o firmware reinicia o chip por comando) |
| `5V` / `GND` | Alimentação do módulo (`VIN` / `GND` do ESP32) |

- Excitação da célula: `E+` no `5V` e `E-` no `GND`, com o mesmo GND do ESP32 e do módulo. Um `E-` solto já causou leitura oscilando.
- Antes de ligar, confira no módulo se os sinais SPI (`SCLK`, `DIN`, `DOUT`, `CS`, `DRDY`) são de 3,3 V.
- O ADS1256 usa um barramento SPI próprio (HSPI), separado do microSD (VSPI).

## 🛠️ Procedimento de Montagem

1. Preparação dos componentes

- Verifique todos os itens da BOM
- Teste ESP32 e cartão SD separadamente antes da montagem final

2. Sequência recomendada

- Conecte alimentação (`VIN` e `GND`)
- Conecte o módulo ADS1256 (SPI) e a célula de carga
- Conecte módulo microSD (SPI)
- Conecte sensor de pressão com divisor resistivo
- Conecte LED de status
- Conecte o botão (`GPIO33`)

3. Divisor resistivo para o sensor de pressão

```
Vsensor (0.5-4.5V) --- R1 (2.2kΩ) --- AIN2 (ADS1256)
                         |
                        R2 (3.3kΩ)
                         |
                        GND
```

Cálculo do divisor:

```
Vout = Vsensor * (R2 / (R1 + R2))

# Exemplo
Vout_max = 4.5V * (3300 / (2200 + 3300)) = 2.7V
```

## 📐 Esquemático

O esquemático atual está no projeto do KiCad: [`teste_estatico.kicad_sch`](../hardware/teste_estatico/kicad/projeto%20kicad/teste_estatico.kicad_sch). Para visualizar, abra no KiCad (versão 10 ou superior).

O esquemático antigo, feito no Fritzing (com o HX711 nos pinos 26/27), foi movido para [`Legacy/`](../Legacy/).

## ⚠️ Considerações de Projeto

- [x] Divisor resistivo no sensor de pressão
- [x] Terra comum entre todos os módulos
- [x] Botão dedicado para TARE
- [x] LED dedicado para indicação de gravação no SD
- [x] Buzzer para sinalização
- [ ] Isolamento contra vibrações
- [ ] Proteção contra poeira e umidade

## 🧪 Testes de Hardware

### Testes pré-montagem

- Continuidade dos cabos
- Tensão de alimentação estável
- Comunicação SPI com microSD
- Leitura de sensores individuais

### Testes pós-montagem

- Botão `GPIO33` realiza TARE
- LED acende durante gravação no SD
- Leituras de empuxo e pressão estáveis
