# Hardware Abstraction Layer (HAL) Architecture — Phase 3

This document details the architecture and design of the **Hardware Abstraction Layer (HAL)** in the Embedded UI Designer.

---

## 1. Objective & Design Philosophy

The primary objective of Phase 3 is to strictly decouple the designer application, UI components, and the Phase 1 Data Source binding model from MCU/board-specific hardware details, probe protocols, and memory register maps.

### Core Architectural Principle:
Components and DataBindings must **never** communicate directly with hardware probes or register addresses:

```
                  EMBEDDED UI DESIGNER
                           │
              ┌────────────┴────────────┐
              │                         │
            Project                  Components
              │                         │
              └───────────┬─────────────┘
                          │
                     DataBinding
                          │
                      DataSource
                          │
                   HardwareManager (Singleton Coordinator)
                          │
              ┌───────────┼────────────┬─────────────┐
              │           │            │             │
           STM32        ESP32     Raspberry Pi      Mock
          Backend      Backend       Backend       Backend
              │           │            │             │
           OpenOCD       JTAG/         Linux       In-Memory
           ST-Link       Serial     GPIO/I2C/SPI   Simulation
              │           │            │             │
              └───────────┴────────────┴─────────────┘
                          │
                   Physical Hardware
```

---

## 2. Core Abstraction Components

### 2.1 Hardware Capabilities (`HardwareCapabilities`)
A data-driven capability struct that eliminates platform-specific `if/else` checks across the UI:
- `gpioInput`, `gpioOutput`
- `adc`, `pwm`
- `uart`, `spi`, `i2c`, `can`
- `reset`, `liveMonitoring`, `rawMemoryAccess`, `atomicBsrr`

### 2.2 Hardware Target (`HardwareTarget`)
Represents an active target device/board configuration within a project:
- **Attributes**: `id`, `name`, `family`, `mcuModel`, `boardId`, `architecture`, `backendType`, `connectionType`, `capabilities`, `pinMappings`, `metadata`.
- **Logical Pin Resolution**: Translates user/project aliases (e.g. `"STATUS_LED"`) to physical hardware pins (e.g. `"PD12"` or `"IO4"`).
- **JSON Serialization**: Completely serializable and loadable without requiring physical hardware to be connected.

### 2.3 Hardware Connection Interface (`HardwareConnection`)
Abstracts debug probes and transport protocols:
- `OpenOcdConnection`: Manages OpenOCD daemon, telnet/GDB pipe, and ST-Link probes.
- `SerialConnection`: Manages USB CDC / UART serial connections for ESP32 and microcontrollers.
- `LinuxSysfsConnection`: Interacts with native Linux kernel drivers (`/sys/class/gpio`, `/dev/i2c-*`, `/dev/spidev*`).
- `MockConnection`: Virtual connection for deterministic unit and regression testing.

### 2.4 Hardware Backend Interface (`HardwareBackend`)
Defines the standard contract implemented by each supported target family:
```cpp
virtual bool readDigital(const QString& pin, bool* outHigh) = 0;
virtual bool writeDigital(const QString& pin, bool high) = 0;
virtual bool readAnalog(const QString& pin, quint32* outRawCount, double* outNormalized) = 0;
virtual bool writePwm(const QString& pin, double dutyPercent) = 0;
virtual bool spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog) = 0;
virtual bool i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog) = 0;
virtual bool resetTarget() = 0;
```

---

## 3. Data Source Resolution Pipeline

When a UI component is bound to a `DataSource` with type `Gpio`, `Adc`, `Pwm`, or `Sensor`:

1. **Component Trigger**: `SwitchComponent` is toggled or `ProgressBar` is updated.
2. **Data Binding**: Evaluates target `DataSource` (`"gpio_status_led"`).
3. **Hardware Reference**: `DataSource` contains `hardwareRef = "STATUS_LED"`.
4. **Target Resolution**: `HardwareManager` resolves `"STATUS_LED"` $\rightarrow$ `"PD12"` via `HardwareTarget::resolvePin()`.
5. **Backend Dispatch**: `HardwareManager` invokes `activeBackend()->writeDigital("PD12", true)`.
6. **Hardware Execution**: `STM32Backend` computes atomic BSRR address (`0x48000C18`, Bit 12) and executes write via OpenOCD probe.
