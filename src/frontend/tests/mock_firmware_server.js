// mock_firmware_server.js - Emulates the compiled firmware REST server (lwIP / esp-idf / mongoose)
import http from "http";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

function createMockFirmwareServer(port = 8080) {
    const server = http.createServer((req, res) => {
        const parsedUrl = new URL(req.url, `http://${req.headers.host || "localhost"}`);
        const pathname = parsedUrl.pathname;

        // CORS headers for cross-origin or local testing
        res.setHeader("Access-Control-Allow-Origin", "*");
        res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.setHeader("Access-Control-Allow-Headers", "Content-Type, Accept");

        if (req.method === "OPTIONS") {
            res.writeHead(204);
            res.end();
            return;
        }

        // 1. Static asset serving for embedded web UI
        if (!pathname.startsWith("/api/v1")) {
            let relativePath = pathname === "/" ? "index.html" : pathname.replace(/^\/+/, "");
            const filePath = path.join(__dirname, "..", relativePath);

            if (fs.existsSync(filePath) && fs.statSync(filePath).isFile()) {
                const ext = path.extname(filePath);
                const mimeTypes = {
                    ".html": "text/html",
                    ".css": "text/css",
                    ".js": "application/javascript",
                    ".json": "application/json",
                    ".png": "image/png"
                };
                res.writeHead(200, { "Content-Type": mimeTypes[ext] || "text/plain" });
                fs.createReadStream(filePath).pipe(res);
                return;
            }
        }

        // 2. REST API: GET /api/v1/device/{deviceId}/gpio/read
        const gpioMatch = pathname.match(/^\/api\/v1\/device\/([^/]+)\/gpio\/read$/);
        if (req.method === "GET" && gpioMatch) {
            const deviceId = gpioMatch[1];
            const pinNumberStr = parsedUrl.searchParams.get("pinNumber");

            if (!pinNumberStr) {
                res.writeHead(400, { "Content-Type": "application/json" });
                res.end(JSON.stringify({ error: "Missing required query parameter: pinNumber" }));
                return;
            }

            const pinNumber = parseInt(pinNumberStr, 10);
            if (isNaN(pinNumber) || pinNumber < 0 || pinNumber > 255) {
                res.writeHead(400, { "Content-Type": "application/json" });
                res.end(JSON.stringify({ error: "pinNumber must be an unsigned integer 0-255" }));
                return;
            }

            // Emulate HAL reading (pin 0 gives 1, others give 0)
            const halValue = pinNumber === 0 ? 1 : (pinNumber % 2);
            res.writeHead(200, { "Content-Type": "application/json" });
            res.end(JSON.stringify({
                deviceId: deviceId,
                pinNumber: pinNumber,
                value: halValue,
                timestamp: Date.now()
            }));
            return;
        }

        // 3. REST API: GET /api/v1/adc/read
        if (req.method === "GET" && pathname === "/api/v1/adc/read") {
            const channelStr = parsedUrl.searchParams.get("channel");
            if (!channelStr) {
                res.writeHead(400, { "Content-Type": "application/json" });
                res.end(JSON.stringify({ error: "Missing required query parameter: channel" }));
                return;
            }

            const channel = parseInt(channelStr, 10);
            if (isNaN(channel) || channel < 0 || channel > 15) {
                res.writeHead(400, { "Content-Type": "application/json" });
                res.end(JSON.stringify({ error: "channel must be an integer 0-15" }));
                return;
            }

            res.writeHead(200, { "Content-Type": "application/json" });
            res.end(JSON.stringify({
                channel: channel,
                value: 2048 + channel * 10,
                timestamp: Date.now()
            }));
            return;
        }

        // 4. REST API: POST /api/v1/pwm/write
        if (req.method === "POST" && pathname === "/api/v1/pwm/write") {
            let body = "";
            req.on("data", chunk => { body += chunk; });
            req.on("end", () => {
                try {
                    const data = JSON.parse(body || "{}");
                    if (data.pin === undefined || data.dutyCycle === undefined) {
                        res.writeHead(400, { "Content-Type": "application/json" });
                        res.end(JSON.stringify({ error: "Missing pin or dutyCycle in payload" }));
                        return;
                    }
                    if (data.dutyCycle < 0 || data.dutyCycle > 100) {
                        res.writeHead(400, { "Content-Type": "application/json" });
                        res.end(JSON.stringify({ error: "dutyCycle must be between 0 and 100" }));
                        return;
                    }

                    res.writeHead(200, { "Content-Type": "application/json" });
                    res.end(JSON.stringify({
                        status: "OK",
                        pin: data.pin,
                        dutyCycle: data.dutyCycle,
                        timestamp: Date.now()
                    }));
                } catch {
                    res.writeHead(400, { "Content-Type": "application/json" });
                    res.end(JSON.stringify({ error: "Invalid JSON payload" }));
                }
            });
            return;
        }

        // Default 404
        res.writeHead(404, { "Content-Type": "application/json" });
        res.end(JSON.stringify({ error: "Not Found", path: pathname }));
    });

    return server;
}

if (process.argv[1] && process.argv[1].endsWith("mock_firmware_server.js")) {
    const port = process.env.PORT || 8080;
    const server = createMockFirmwareServer(port);
    server.listen(port, () => {
        console.log(`Mock Embedded Firmware REST Server listening on port ${port}`);
    });
}

export { createMockFirmwareServer };
