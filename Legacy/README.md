# 🗄️ Legacy — Arquivos Antigos

## 📋 Sobre

Esta pasta guarda arquivos que **não fazem mais parte do sistema atual**, mas que foram mantidos como histórico. Nada aqui é usado pelo firmware, pelas ferramentas de calibração ou pela documentação principal.

Os arquivos mantêm o **caminho original** dentro de `Legacy/`. Por exemplo, `extras/old/old.ino` virou `Legacy/extras/old/old.ino`.

> ⚠️ Não use estes arquivos em montagens novas: a pinagem e o hardware são antigos.

## 📁 Conteúdo

| Caminho | O que é | Por que saiu |
|---|---|---|
| `extras/old/` | Firmware antigo (RTC DS3231, relé, HX711 em 26/27) | Substituído por `firmware/firmware.ino` |
| `software/config-load-cell/Config.ino`, `Verify.ino` | Sketches antigos para ler o valor bruto e verificar o fator do HX711 | Substituídos pelo app de calibração (`fator_de_calibracao/`) |
| `software/config-load-cell/Calibrar.py`, `DadosSerial.py` | Scripts antigos que salvavam leituras brutas e calculavam o fator offline | Substituídos pelo app de calibração |
| `software/config-load-cell/Dados.txt`, `Dados-Teste-Estatico/` | Leituras brutas do HX711 usadas na calibração antiga | Não valem para o ADS1256 |
| `software/Fator manual/` | Sketch de calibração manual | Cópia duplicada de `software/fatordecalibracao/` |
| `docs/Esquematico.png`, `hardware/Untitled Sketch 2.fzz` | Esquemático do Fritzing (HX711 em 26/27, dois botões, sem buzzer) | Substituído pelo esquemático do KiCad |
| `hardware/teste_estatico/Fusion 360/` | PCB feita no Fusion 360 (2026-05) | Substituída pelo projeto KiCad |
| `hardware/teste_estatico/kicad/gerber/` | Gerbers de 2026-05-21 | Os atuais (2026-07-09) estão em `hardware/teste_estatico/kicad/projeto kicad/` |
