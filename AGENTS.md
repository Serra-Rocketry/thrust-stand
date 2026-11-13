# AGENTS.md — Thrust Stand

## Project Overview

ESP32-based embedded system for rocket engine static test data acquisition (thrust + pressure), with SD/LittleFS storage and Serial/Bluetooth communication. Monorepo containing firmware (C++/Arduino), desktop calibration tools (Python/PySide6), hardware design files (KiCad/Fusion360), and documentation.

## Build & Run Commands

### Firmware (Arduino/ESP32)

No CLI build system is configured. Firmware is built via the Arduino IDE or Arduino CLI:

```bash
# Install Arduino CLI (if not present)
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Compile (requires ESP32 board package installed)
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/

# Upload
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 firmware/

# Serial monitor
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

Required libraries: `Pushbutton` (Pololu), `BluetoothSerial` (built-in), `Preferences` (built-in). The ADS1256 driver is local (`firmware/ADS1256.h`).

### Python Desktop Tools

```bash
cd software/config-load-cell/fator_de_calibracao/
pip install -r requirements.txt
python fator_de_calibracao_main.py   # GUI
python fator_de_calibracao_cli.py    # CLI
```

### Testing

No test framework or test suite exists, and there is no `tests/` directory yet. If adding tests, create it:

- Python: use `pytest`. Run with `pytest tests/ -v` or a single test with `pytest tests/test_foo.py::test_bar -v`.
- C++/Arduino: use PlatformIO test runner or Arduino CLI with a test sketch.

### Linting & Formatting

No linting or formatting tools are configured. If introducing them:

- Python: `ruff check .` and `ruff format .` (or `black .` + `flake8 .`)
- C++: `clang-format -i firmware/*.ino firmware/*.h`

## Code Style Guidelines

### C++ / Arduino (firmware/)

**Naming:**
- `#define` constants and enum values: `UPPER_SNAKE_CASE` (`LED_PIN`, `STORAGE_SD`)
- Global variables and functions: `camelCase` (`loadFactor`, `setupStorageAndFile`)
- Class methods: `camelCase` (`readMPa`, `begin`)
- Class member variables: prefixed with underscore (`_pin`, `_resistor1`)

**Includes:**
- System/library headers with angle brackets: `#include <Wire.h>`
- Local headers with quotes: `#include "Pressure.h"`
- Order: standard Arduino libs first, then third-party, then local

**Formatting:**
- Opening brace on same line for control flow (`if`, `for`, `while`)
- Opening brace on new line for function definitions (`void setup()`)
- Use `#define` for pin assignments, `const` for computed values
- Use Arduino `String` class (not `std::string`)

**Error handling:**
- Functions return `bool` to indicate success/failure
- Critical failures in `setup()` call `ESP.restart()` after a delay
- Non-critical failures print to Serial and toggle LED
- No exceptions (standard for embedded)

**General:**
- Comments and user-facing strings in Brazilian Portuguese
- Use `Preferences` API for persistent NVS storage
- Use enum for polymorphic storage selection (SD vs LittleFS)
- Keep headers as single-class implementations with include guards

### Python (software/)

**Naming:**
- Classes: `PascalCase` (`Receiver`, `SamplerThread`, `Calibrator`)
- Functions/methods: `snake_case` (`get_samples`, `try_parse_int`)
- Variables: `snake_case` (`sample_count`, `calibration_factor`)
- Constants: `UPPER_SNAKE_CASE` (if any)

**Type hints:**
- Always use type hints in function signatures
- Use modern syntax: `int | None` (not `Optional[int]`), `list[int]` (not `List[int]`)
- Example: `def try_parse_int(line: str) -> int | None:`

**Imports:**
- Standard library first, then third-party, then local/relative
- One module per line: `import serial` and `import serial.tools.list_ports`
- Within the package, use relative imports: `import fator_de_calibracao_cli as fact_cli`

**Formatting:**
- 4-space indentation (PEP 8)
- Docstrings in triple double-quotes for public functions/classes
- Comments in Brazilian Portuguese for user-facing text

**Error handling:**
- Use `try/except` for serial communication (`serial.SerialException`)
- Defensive checks before operations: `if com is None`, `if not samples`
- Use `print()` for debug output; `logging` module is imported but sparingly used

**Qt/PySide6:**
- Use `QThread` subclass for background serial I/O (never block the UI thread)
- Use Qt signals (`Signal`) for thread-safe UI updates
- Never manually edit auto-generated UI files (`ui_fator_de_calibracao.py`)

### Documentation

- All docs are in Brazilian Portuguese (Markdown)
- Technical docs live in `docs/` (API.md, FIRMWARE.md, HARDWARE.md, TROUBLESHOOTING.md, EVOLUÇÃO_CAIXA.md, V3_DETALHES.md)
- Follow existing doc structure and emoji conventions when adding new docs
- Hardware photos go in `docs/imagens/` with descriptive names: `caixa_v1_frontal_superior.jpg` (version + angle, no spaces)
- Compress images before committing: `convert input.jpg -resize 1200x -quality 80 -strip output.jpg`
- Use HTML tables for side-by-side images (3 per row, 300px width), isolated images at 500px
- Videos go in `docs/videos/` with descriptive names: `caixa_v1_teste_estatico.mp4` (version + type, no spaces)
- Use Git LFS for video files (`git lfs track "*.mp4"`)
- Convert videos to animated AVIF for inline playback: `ffmpeg -i input.mp4 -vcodec libsvtav1 -vf "scale=500:-2" -r 15 -crf 30 -preset 8 output.avif`
- Animated AVIFs go in `docs/imagens/`, embed with `![Descrição](./imagens/arquivo.avif)`
- Keep MP4 originals in `docs/videos/` and link as fallback: `[Versão com áudio (MP4)](./videos/arquivo.mp4)`

## Repository Conventions

- **Branching:** feature branches from main (`feature/AmazingFeature`)
- **Remotes:** `origin` (oangelo/thrust-stand) + `upstream` (Serra-Rocketry/thrust-stand) — push to both when ready
- **License:** GPL v3 — all contributions must be compatible
- **No secrets:** never commit credentials, API keys, or `.env` files
- **No binary artifacts:** avoid committing compiled firmware binaries or build outputs
- **Hardware files:** KiCad and Fusion 360 source files are tracked; generated manufacturing files are acceptable in `hardware/`

## Key File Reference

| Path | Description |
|---|---|
| `firmware/firmware.ino` | Main firmware entry point (406 lines) |
| `firmware/ADS1256.h` | ADS1256 24-bit ADC driver (header-only, SPI) |
| `firmware/Pressure.h` | PressureSensor class: voltage -> pressure (header-only) |
| `software/config-load-cell/fator_de_calibracao/fator_de_calibracao_main.py` | Qt GUI calibration app |
| `software/config-load-cell/fator_de_calibracao/fator_de_calibracao_cli.py` | Serial communication + calibration logic |
| `software/config-load-cell/fator_de_calibracao/ui_fator_de_calibracao.py` | Auto-generated Qt UI (DO NOT EDIT) |
| `docs/API.md` | Serial/Bluetooth protocol documentation |
| `docs/FIRMWARE.md` | Firmware architecture and commands |
| `docs/HARDWARE.md` | Schematic, BOM, pinout |
| `docs/EVOLUÇÃO_CAIXA.md` | Evolution of the physical test stand structure (3 versions) |
| `docs/V3_DETALHES.md` | V3 test stand detailed specs (load cell, linear guide, resolution) |

## Data Storage Conventions

- **Static test data:** motor-specific test data (thrust curves, pressure logs) belongs in [Serra-Rocketry/motor](https://github.com/Serra-Rocketry/motor), not in this repo.
- **This repo:** contains only the acquisition system (firmware, hardware, calibration tools).
- **Calibration data:** calibration reference data only, never full test logs. The old HX711 raw data lives in `Legacy/software/config-load-cell/Dados-Teste-Estatico/`.

## Common Pitfalls

- The `ui_fator_de_calibracao.py` file is auto-generated from `fator_de_calibracao.ui` — edit the `.ui` file in Qt Designer instead.
- Load cell and pressure are both read by the ADS1256, each channel with its own PGA and self-calibration (`ADS1256Channel`). Don't switch PGA without recalibrating that channel.
- The firmware streams raw integers during calibration mode (`configMode`) — the Python tools expect this format.
- LittleFS is the fallback when SD card initialization fails; code must handle both storage types.
