// api-client.js - Type-safe REST client validating against ApiSchema

class ApiClient {
    constructor(baseUrl = "http://localhost:8080/api/v1") {
        this.baseUrl = baseUrl.replace(/\/+$/, "");
        this.schema = null;
        this.typeRegistry = null;
    }
    
    // Load schema from device, URL, or local object
    async loadSchema(schemaUrlOrObject) {
        try {
            if (typeof schemaUrlOrObject === "string") {
                const response = await fetch(schemaUrlOrObject);
                if (!response.ok) {
                    throw new Error(`Failed to fetch schema: HTTP ${response.status}`);
                }
                this.schema = await response.json();
            } else if (typeof schemaUrlOrObject === "object" && schemaUrlOrObject !== null) {
                this.schema = schemaUrlOrObject;
            } else {
                throw new Error("Invalid schema argument");
            }
            return this.schema;
        } catch (error) {
            console.error("Failed to load schema:", error);
            throw error;
        }
    }
    
    // Validate request parameters against schema constraints
    validateRequest(operationId, params = {}) {
        if (!this.schema || !this.schema.apiSchema || !Array.isArray(this.schema.apiSchema.endpoints)) {
            // If schema not loaded yet, allow request or throw
            return true;
        }

        const endpoint = this.schema.apiSchema.endpoints.find(e => e.operationId === operationId);
        if (!endpoint) {
            throw new Error(`Endpoint operation '${operationId}' not found in schema`);
        }
        
        // Check required parameters
        for (const param of endpoint.parameters) {
            if (param.required && (params[param.paramName] === undefined || params[param.paramName] === null || params[param.paramName] === "")) {
                throw new Error(`Required parameter missing: ${param.paramName}`);
            }
            
            // Type validation
            const paramValue = params[param.paramName];
            if (paramValue !== undefined && !this._isValidType(paramValue, param.typeId)) {
                throw new Error(`Invalid type for ${param.paramName}: expected ${param.typeId}, got ${typeof paramValue} (${paramValue})`);
            }
        }
        
        return true;
    }
    
    _isValidType(value, typeId) {
        switch (typeId) {
            case "uint8":
                return Number.isInteger(value) && value >= 0 && value <= 255;
            case "uint16":
                return Number.isInteger(value) && value >= 0 && value <= 65535;
            case "uint32":
                return Number.isInteger(value) && value >= 0;
            case "int8":
                return Number.isInteger(value) && value >= -128 && value <= 127;
            case "int16":
                return Number.isInteger(value) && value >= -32768 && value <= 32767;
            case "int32":
                return Number.isInteger(value);
            case "float":
            case "double":
                return typeof value === "number" && !isNaN(value);
            case "string":
                return typeof value === "string";
            case "bool":
                return typeof value === "boolean";
            default:
                return true;
        }
    }
    
    // Core HTTP request execution
    async request(method, path, data = null, customSignal = null) {
        const normalizedPath = path.startsWith("/") ? path : `/${path}`;
        const url = `${this.baseUrl}${normalizedPath}`;
        
        const options = {
            method: method.toUpperCase(),
            headers: {
                "Accept": "application/json"
            }
        };

        if (customSignal) {
            options.signal = customSignal;
        }
        
        if (data !== null && data !== undefined) {
            options.headers["Content-Type"] = "application/json";
            options.body = JSON.stringify(data);
        }
        
        try {
            const response = await fetch(url, options);
            const contentType = response.headers.get("content-type") || "";
            
            let responseData;
            if (contentType.includes("application/json")) {
                responseData = await response.json();
            } else {
                const text = await response.text();
                try {
                    responseData = JSON.parse(text);
                } catch {
                    responseData = { message: text };
                }
            }
            
            if (!response.ok) {
                const errMsg = responseData.error || responseData.message || `HTTP ${response.status}`;
                const err = new Error(`API Error: ${response.status} - ${errMsg}`);
                err.status = response.status;
                err.data = responseData;
                throw err;
            }
            
            return {
                status: response.status,
                data: responseData,
                timestamp: new Date().toISOString()
            };
        } catch (error) {
            if (error.name === "AbortError") {
                throw new Error("Request timed out");
            }
            throw error;
        }
    }
    
    // Convenience helper methods
    async readGpio(deviceId, pinNumber) {
        this.validateRequest("readGPIO", { deviceId, pinNumber });
        return this.request("GET", `/device/${encodeURIComponent(deviceId)}/gpio/read?pinNumber=${pinNumber}`);
    }
    
    async readAdc(channel) {
        this.validateRequest("readADC", { channel });
        return this.request("GET", `/adc/read?channel=${channel}`);
    }
    
    async setPwm(pin, dutyCycle) {
        this.validateRequest("setPWM", { pin, dutyCycle });
        return this.request("POST", "/pwm/write", { pin, dutyCycle });
    }
}

// Support Node.js CommonJS and ES environments
if (typeof module !== "undefined" && module.exports) {
    module.exports = { ApiClient };
}
export { ApiClient };
