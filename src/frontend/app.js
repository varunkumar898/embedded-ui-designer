// app.js - Frontend application controller

const apiClient = new ApiClient("http://localhost:8080/api/v1");

document.addEventListener("DOMContentLoaded", async () => {
    // Initialize API client with schema
    try {
        await apiClient.loadSchema("schema.json");
        updateConnectionStatus(true);
        logDebug("Schema loaded successfully. Ready for operations.");
    } catch (error) {
        console.warn("Schema initialization error (running in standalone mode):", error.message);
        updateConnectionStatus(false);
    }
    
    // Register UI event handlers
    const readGpioBtn = document.getElementById("readGpioBtn");
    if (readGpioBtn) readGpioBtn.addEventListener("click", handleReadGpio);

    const readAdcBtn = document.getElementById("readAdcBtn");
    if (readAdcBtn) readAdcBtn.addEventListener("click", handleReadAdc);

    const setPwmBtn = document.getElementById("setPwmBtn");
    if (setPwmBtn) setPwmBtn.addEventListener("click", handleSetPwm);
    
    // PWM duty cycle slider display update
    const pwmSlider = document.getElementById("pwmDutyCycle");
    if (pwmSlider) {
        pwmSlider.addEventListener("input", (e) => {
            const display = document.getElementById("pwmDutyCycleDisplay");
            if (display) display.textContent = e.target.value + "%";
        });
    }
    
    // Debug panel toggle & clear
    const settingsBtn = document.getElementById("settingsBtn");
    if (settingsBtn) {
        settingsBtn.addEventListener("click", () => {
            const debugPanel = document.getElementById("debugPanel");
            if (debugPanel) {
                debugPanel.style.display = debugPanel.style.display === "none" ? "flex" : "none";
            }
        });
    }

    const clearDebugBtn = document.getElementById("clearDebugBtn");
    if (clearDebugBtn) {
        clearDebugBtn.addEventListener("click", () => {
            const debugLog = document.getElementById("debugLog");
            if (debugLog) debugLog.innerHTML = "";
        });
    }
});

async function handleReadGpio() {
    const deviceIdInput = document.getElementById("deviceId");
    const pinNumberInput = document.getElementById("pinNumber");
    
    const deviceId = deviceIdInput ? deviceIdInput.value.trim() : "";
    const rawPin = pinNumberInput ? pinNumberInput.value : "";
    const pinNumber = parseInt(rawPin, 10);
    
    // Client-side quick check
    if (!deviceId) {
        displayError("gpio", "Device ID is required");
        logDebug("GPIO Read Error: Device ID is required");
        return;
    }
    if (isNaN(pinNumber) || pinNumber < 0 || pinNumber > 255) {
        displayError("gpio", "Pin number must be an integer between 0 and 255");
        logDebug("GPIO Read Error: Invalid pin number");
        return;
    }

    try {
        logDebug(`Sending GET /device/${deviceId}/gpio/read?pinNumber=${pinNumber}`);
        const result = await apiClient.readGpio(deviceId, pinNumber);
        displayResult("gpio", result);
        logDebug(`GPIO Read Success: ${JSON.stringify(result.data)}`);
    } catch (error) {
        displayError("gpio", error.message);
        logDebug(`GPIO Read Failed: ${error.message}`);
    }
}

async function handleReadAdc() {
    const adcChannelInput = document.getElementById("adcChannel");
    const rawChannel = adcChannelInput ? adcChannelInput.value : "";
    const channel = parseInt(rawChannel, 10);
    
    if (isNaN(channel) || channel < 0 || channel > 15) {
        displayError("adc", "ADC Channel must be between 0 and 15");
        logDebug("ADC Read Error: Invalid channel");
        return;
    }

    try {
        logDebug(`Sending GET /adc/read?channel=${channel}`);
        const result = await apiClient.readAdc(channel);
        displayResult("adc", result);
        logDebug(`ADC Read Success: ${JSON.stringify(result.data)}`);
    } catch (error) {
        displayError("adc", error.message);
        logDebug(`ADC Read Failed: ${error.message}`);
    }
}

async function handleSetPwm() {
    const pwmPinInput = document.getElementById("pwmPin");
    const pwmDutyCycleInput = document.getElementById("pwmDutyCycle");
    
    const pin = parseInt(pwmPinInput ? pwmPinInput.value : "", 10);
    const dutyCycle = parseInt(pwmDutyCycleInput ? pwmDutyCycleInput.value : "", 10);
    
    if (isNaN(pin) || pin < 0 || pin > 255) {
        displayError("pwm", "PWM Pin must be between 0 and 255");
        logDebug("PWM Write Error: Invalid pin");
        return;
    }
    if (isNaN(dutyCycle) || dutyCycle < 0 || dutyCycle > 100) {
        displayError("pwm", "Duty cycle must be between 0 and 100%");
        logDebug("PWM Write Error: Invalid duty cycle");
        return;
    }

    try {
        logDebug(`Sending POST /pwm/write { pin: ${pin}, dutyCycle: ${dutyCycle} }`);
        const result = await apiClient.setPwm(pin, dutyCycle);
        displayResult("pwm", result);
        logDebug(`PWM Write Success: ${JSON.stringify(result.data)}`);
    } catch (error) {
        displayError("pwm", error.message);
        logDebug(`PWM Write Failed: ${error.message}`);
    }
}

function displayResult(type, result) {
    const panel = document.getElementById(`${type}Result`);
    if (panel) panel.style.display = "block";
    const jsonPre = document.getElementById(`${type}ResultJson`);
    if (jsonPre) jsonPre.textContent = JSON.stringify(result.data, null, 2);
    const statusP = document.getElementById(`${type}ResultStatus`);
    if (statusP) {
        statusP.textContent = `Status: ${result.status} (OK) - ${result.timestamp}`;
        statusP.style.color = "var(--color-success, #28a745)";
    }
}

function displayError(type, error) {
    const panel = document.getElementById(`${type}Result`);
    if (panel) panel.style.display = "block";
    const jsonPre = document.getElementById(`${type}ResultJson`);
    if (jsonPre) jsonPre.textContent = JSON.stringify({ error: error }, null, 2);
    const statusP = document.getElementById(`${type}ResultStatus`);
    if (statusP) {
        statusP.textContent = "Error: Validation or Network Failure";
        statusP.style.color = "var(--color-danger, #dc3545)";
    }
}

function updateConnectionStatus(connected) {
    const status = document.getElementById("connectionStatus");
    if (!status) return;
    status.className = "status-indicator " + (connected ? "connected" : "disconnected");
    status.textContent = connected ? "Connected" : "Disconnected";
}

function logDebug(message) {
    const log = document.getElementById("debugLog");
    if (!log) return;
    const entry = document.createElement("div");
    entry.className = "log-entry";
    entry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
    log.appendChild(entry);
    log.scrollTop = log.scrollHeight;
}
