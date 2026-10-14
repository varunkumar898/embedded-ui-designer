# Embedded UI Designer: REST API Reference (OpenAPI 3.0)

## Base URL
- Production: `https://<device-ip>:8443/api/v1`
- Development / Insecure: `http://<device-ip>:8080/api/v1`

---

## Security Schemes
- **API Key**: `Authorization: ApiKey <KeyId>` and `X-API-Secret: <Secret>`
- **Bearer Token**: `Authorization: Bearer <Token>`
- **HMAC Signature**: `X-Device-Id: <DeviceId>`, `X-Signature: <HmacSignature>`, `X-Nonce: <Nonce>`

---

## Endpoints

### 1. Read GPIO State
* **Method**: `GET`
* **Path**: `/api/v1/readGpio`
* **Query Parameters**:
  - `pin` (integer, required): Pin number on target MCU (e.g. 5)
* **Response (200 OK)**:
  ```json
  {
    "status": "success",
    "pin": 5,
    "value": 1
  }
  ```

### 2. Read ADC Channel
* **Method**: `GET`
* **Path**: `/api/v1/readAdc`
* **Query Parameters**:
  - `channel` (integer, required): ADC channel number (e.g. 0)
* **Response (200 OK)**:
  ```json
  {
    "status": "success",
    "channel": 0,
    "value": 3200
  }
  ```

### 3. Set PWM Duty Cycle
* **Method**: `POST`
* **Path**: `/api/v1/setPwm`
* **Request Body**:
  ```json
  {
    "pin": 3,
    "dutyCycle": 60
  }
  ```
* **Response (200 OK)**:
  ```json
  {
    "status": "success",
    "pin": 3,
    "dutyCycle": 60
  }
  ```

### 4. Health Check
* **Method**: `GET`
* **Path**: `/health`
* **Response (200 OK)**:
  ```json
  {
    "status": "ok",
    "uptimeSeconds": 86400,
    "version": "1.0.0"
  }
  ```

### 5. Prometheus Metrics Exporter
* **Method**: `GET`
* **Path**: `/metrics`
* **Response (200 OK)**: Text format (Prometheus exposition format).
