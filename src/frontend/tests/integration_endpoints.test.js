// tests/integration_endpoints.test.js

const endpointTests = {
    name: "API Endpoint & Full-Stack Integration Tests",
    tests: [
        {
            name: "readGpio returns uint8 value",
            fn: async (tf) => {
                const response = await fetch(`${tf.apiBaseUrl}/device/dev1/gpio/read?pinNumber=0`);
                tf.assert(response.ok, `HTTP status was ${response.status}`);
                
                const data = await response.json();
                tf.assert("value" in data, "Response missing 'value' field");
                tf.assert(typeof data.value === "number", "value must be number");
                tf.assert(data.value >= 0 && data.value <= 255, "value must be 0-255");
                tf.assert("timestamp" in data, "Response missing 'timestamp' field");
            }
        },
        {
            name: "readGpio with missing parameter returns 400",
            fn: async (tf) => {
                const response = await fetch(`${tf.apiBaseUrl}/device/dev1/gpio/read`);
                tf.assertEqual(response.status, 400, "Missing parameter should return 400");
                const data = await response.json();
                tf.assert(data.error !== undefined, "Expected error message in response");
            }
        },
        {
            name: "readAdc returns uint16 value",
            fn: async (tf) => {
                const response = await fetch(`${tf.apiBaseUrl}/adc/read?channel=0`);
                tf.assert(response.ok, `HTTP status was ${response.status}`);
                
                const data = await response.json();
                tf.assert("value" in data, "Response missing 'value' field");
                tf.assert(typeof data.value === "number", "value must be number");
                tf.assert(data.value >= 0 && data.value <= 65535, "value must be 0-65535");
            }
        },
        {
            name: "setPwm with valid parameters returns 200",
            fn: async (tf) => {
                const payload = { pin: 10, dutyCycle: 75 };
                const response = await fetch(`${tf.apiBaseUrl}/pwm/write`, {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify(payload)
                });
                tf.assertEqual(response.status, 200, "Valid PWM request should return 200");
                
                const data = await response.json();
                tf.assert("status" in data, "Response missing 'status' field");
                tf.assertEqual(data.status, "OK", "Status should be OK");
                tf.assertEqual(data.dutyCycle, 75, "Duty cycle should be 75");
            }
        },
        {
            name: "setPwm with invalid dutyCycle returns 400",
            fn: async (tf) => {
                const payload = { pin: 10, dutyCycle: 150 }; // >100 is invalid
                const response = await fetch(`${tf.apiBaseUrl}/pwm/write`, {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify(payload)
                });
                tf.assertEqual(response.status, 400, "Invalid duty cycle should return 400");
            }
        },
        {
            name: "Multiple concurrent requests succeed",
            fn: async (tf) => {
                const requests = [
                    fetch(`${tf.apiBaseUrl}/device/dev1/gpio/read?pinNumber=0`),
                    fetch(`${tf.apiBaseUrl}/device/dev1/gpio/read?pinNumber=1`),
                    fetch(`${tf.apiBaseUrl}/device/dev1/gpio/read?pinNumber=2`),
                    fetch(`${tf.apiBaseUrl}/adc/read?channel=1`),
                    fetch(`${tf.apiBaseUrl}/adc/read?channel=2`)
                ];
                
                const responses = await Promise.all(requests);
                for (const response of responses) {
                    tf.assert(response.ok, `Concurrent request failed with status: ${response.status}`);
                }
            }
        },
        {
            name: "Response serialization is valid JSON",
            fn: async (tf) => {
                const response = await fetch(`${tf.apiBaseUrl}/device/dev1/gpio/read?pinNumber=0`);
                const text = await response.text();
                
                let parsed;
                try {
                    parsed = JSON.parse(text);
                } catch {
                    throw new Error("Response is not valid JSON");
                }
                tf.assert(typeof parsed === "object" && parsed !== null, "Parsed JSON must be an object");
            }
        },
        {
            name: "Request timeout after 5 seconds",
            fn: async (tf) => {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 5000);
                
                try {
                    const response = await fetch(`${tf.apiBaseUrl}/device/dev1/gpio/read?pinNumber=0`, {
                        signal: controller.signal
                    });
                    clearTimeout(timeoutId);
                    tf.assert(response.ok, "Request should complete within 5 seconds");
                } catch (error) {
                    if (error.name === "AbortError") {
                        throw new Error("Request timed out (exceeded 5 seconds)");
                    }
                    throw error;
                }
            }
        }
    ]
};

if (typeof module !== "undefined" && module.exports) {
    module.exports = { endpointTests };
}
export { endpointTests };
