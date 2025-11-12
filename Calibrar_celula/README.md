# Explicação do Sistema

Este sistema é composto por quatro componentes principais:

1. **Config.ino**: Este código roda em um ESP32 e é responsável por enviar as leituras da célula de carga.
2. **DadosSerial.py**: Este script lê as leituras da célula de carga enviadas pelo ESP32.
3. **Calibrar.py**: Este script realiza o cálculo do fator de calibração da célula de carga.
4. **Verify.ino**: Este código verifica a precisão da célula de carga após a calibração.

- **Config.ino**: Configura o ESP32 para coletar dados da célula de carga e enviá-los via comunicação serial.
- **DadosSerial.py**: Recebe os dados enviados pelo ESP32 e os salva para análise posterior.
- **Calibrar.py**: Utiliza os dados salvos para calcular o fator de calibração necessário para ajustar a precisão da célula de carga.
- **Verify.ino**: Verifica a precisão da célula de carga após a calibração, exibindo as leituras no monitor serial.

## Passo a Passo para Configurar uma Célula do Zero

1. **Montagem do Hardware**:

   - Conecte a célula de carga ao amplificador HX711.
   - Conecte o amplificador HX711 ao ESP32.

2. **Configuração do ESP32**:

   - Carregue o código `Config.ino` no ESP32 usando a IDE Arduino.
   - Verifique se o ESP32 está enviando dados da célula de carga via comunicação serial.

3. **Leitura dos Dados**:

   - Adicione um peso conhecido na célula de carga.
   - Execute o script `DadosSerial.py` para ler os dados enviados pelo ESP32.
   - Certifique-se de que os dados estão sendo recebidos corretamente.

4. **Calibração da Célula de Carga**:

   - Verifique o arquivo `Dados.txt`.
   - Execute o script `Calibrar.py` para calcular o fator de calibração.
   - Informe o peso usado anteriormente.

5. **Verificação Final**:
   - Carregue o código `Verify.ino` no ESP32 para verificar a precisão da célula de carga.
   - Teste a célula de carga com pesos conhecidos para garantir a precisão.
   - Faça ajustes adicionais no fator de calibração, se necessário.
