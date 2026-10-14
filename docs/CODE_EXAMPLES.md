# Multi-Language Code Examples

This document demonstrates client and firmware implementations for interacting with the Embedded UI Designer API across Python, JavaScript, C++, C, and Rust.

---

## 1. Python (Requests Client)

```python
import requests
import json

BASE_URL = "http://192.168.1.100:8080/api/v1"
API_KEY = "device-admin-key"
API_SECRET = "secret-token-xyz"

headers = {
    "Authorization": f"ApiKey {API_KEY}",
    "X-API-Secret": API_SECRET,
    "Content-Type": "application/json"
}

# 1. Read GPIO pin 5
response = requests.get(f"{BASE_URL}/readGpio?pin=5", headers=headers)
print("GPIO 5 State:", response.json())

# 2. Read ADC channel 0
adc_resp = requests.get(f"{BASE_URL}/readAdc?channel=0", headers=headers)
print("ADC 0 Value:", adc_resp.json())

# 3. Set PWM duty cycle
pwm_payload = {"pin": 3, "dutyCycle": 65}
pwm_resp = requests.post(f"{BASE_URL}/setPwm", headers=headers, json=pwm_payload)
print("PWM Update Status:", pwm_resp.status_code)
```

---

## 2. JavaScript / TypeScript (Fetch API)

```javascript
const BASE_URL = 'http://192.168.1.100:8080/api/v1';
const API_KEY = 'device-admin-key';
const API_SECRET = 'secret-token-xyz';

const headers = {
  'Authorization': `ApiKey ${API_KEY}`,
  'X-API-Secret': API_SECRET,
  'Content-Type': 'application/json'
};

async function controlDevice() {
  // Read GPIO
  const gpioRes = await fetch(`${BASE_URL}/readGpio?pin=5`, { headers });
  const gpioData = await gpioRes.json();
  console.log('GPIO Pin 5:', gpioData);

  // Set PWM
  const pwmRes = await fetch(`${BASE_URL}/setPwm`, {
    method: 'POST',
    headers,
    body: JSON.stringify({ pin: 3, dutyCycle: 75 })
  });
  console.log('PWM Response Status:', pwmRes.status);
}

controlDevice();
```

---

## 3. C++ (C++17 Firmware Handler)

```cpp
#include <iostream>
#include <string>
#include <map>
#include "security/SecurityConfig.h"
#include "performance/ResponseCache.h"

void handleReadGpio(uint32_t pin, security::SecurityConfig& secConfig) {
    // Validate access
    if (!secConfig.isApiKeyAllowed("device-admin-key", "/api/v1/readGpio")) {
        std::cerr << "Access denied to readGpio" << std::endl;
        return;
    }

    // Direct hardware register read
    uint8_t value = 1; // e.g. HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
    std::cout << "{\"pin\":" << pin << ",\"value\":" << static_cast<int>(value) << "}" << std::endl;
}
```

---

## 4. Bare-Metal C (STM32 HAL)

```c
#include <stdint.h>
#include <stdbool.h>
#include "stm32f0xx_hal.h"

uint8_t read_gpio_pin(uint16_t pin_num) {
    GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, (uint16_t)(1 << pin_num));
    return (state == GPIO_PIN_SET) ? 1 : 0;
}

void set_pwm_duty(uint32_t channel, uint8_t duty_cycle) {
    // TIM3 channel duty cycle update
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty_cycle);
}
```

---

## 5. Memory-Safe Rust (embedded-hal)

```rust
#![no_std]
#![no_main]

use embedded_hal::digital::v2::InputPin;
use stm32f0xx_hal::{pac, prelude::*};

#[entry]
fn main() -> ! {
    let dp = pac::Peripherals::take().unwrap();
    let gpioa = dp.GPIOA.split();
    let pin5 = gpioa.pa5.into_pull_up_input();

    loop {
        let is_high = pin5.is_high().unwrap_or(false);
        // Process state without heap allocation
    }
}
```
