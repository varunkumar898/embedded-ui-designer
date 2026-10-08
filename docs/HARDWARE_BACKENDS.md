# Hardware Backends Reference — Phase 3

This document details the concrete backends implemented in the **Hardware Abstraction Layer (HAL)** and guides how to implement a future hardware backend.

---

## 1. Concrete Backend Implementations

### 1.1 `STM32Backend`
- **Family**: STM32
- **Transport**: `OpenOcdConnection` (ST-Link probe)
- **Features**:
  - **GPIO**: Atomic Bit Set/Reset Register (BSRR) writes (`0x48000018` for GPIOA etc.).
  - **ADC**: Periodic reading of ADC registers (`ADC1_DR`) with fallback to simulation when disconnected.
  - **PWM**: Timer channel duty configuration.
  - **SPI**: Raw byte transfer with loopback verification.
  - **I2C**: Bus scan and multi-sensor discovery.
  - **Status**: **IMPLEMENTED & TESTED**

### 1.2 `ESP32Backend`
- **Family**: ESP32
- **Transport**: `SerialConnection` (USB CDC / UART)
- **Features**:
  - **GPIO**: IO0 to IO48 digital state control (**SUPPORTED**).
  - **ADC**: 12-bit ADC channels (**SUPPORTED**).
  - **PWM**: LEDC duty cycle control (**SUPPORTED**).
  - **UART**: Packet transmission (**SUPPORTED**).
  - **SPI / I2C**: Software transfer (**EXPERIMENTAL**).
  - **CAN**: Multi-node transceiver (**NOT IMPLEMENTED in Phase 3**).
  - **Status**: **IMPLEMENTED & TESTED**

### 1.3 `RaspberryPiBackend`
- **Family**: Raspberry Pi / Linux SBC
- **Transport**: `LinuxSysfsConnection`
- **Features**:
  - **GPIO**: Linux sysfs / gpiod header pins (**SUPPORTED**).
  - **I2C**: `/dev/i2c-1` access (**SUPPORTED**).
  - **SPI**: `/dev/spidev0.0` access (**SUPPORTED**).
  - **ADC**: Reports unsupported (no native on-chip ADC) (**SUPPORTED / CORRECTLY REPORTED**).
  - **Status**: **IMPLEMENTED & TESTED**

### 1.4 `MockBackend`
- **Family**: Mock
- **Transport**: `MockConnection`
- **Features**:
  - Full in-memory deterministic simulation of digital IO, analog ADC values, PWM duty, I2C scan, and disconnect triggers.
  - **Status**: **IMPLEMENTED & TESTED**

---

## 2. Implementing a Future Hardware Backend

To add a new backend (e.g. `NXPBackend` or `RP2040Backend`):

1. **Create Backend Class**: Inherit from `Hardware::HardwareBackend`.
2. **Implement Required Methods**: `readDigital()`, `writeDigital()`, `readAnalog()`, `writePwm()`, `capabilities()`, `availablePins()`.
3. **Register in `HardwareManager`**:
   ```cpp
   HardwareManager::instance().registerBackend(std::make_shared<NXPBackend>());
   ```
4. **No UI or Component Changes Required**: All UI components, canvas scenes, properties panels, and DataBindings will immediately work with the new backend through `DataSource` resolution!
