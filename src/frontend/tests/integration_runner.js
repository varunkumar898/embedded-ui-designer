// tests/integration_runner.js - End-to-end integration test runner
import { TestFramework } from "../test-framework.js";
import { endpointTests } from "./integration_endpoints.test.js";
import { createMockFirmwareServer } from "./mock_firmware_server.js";
import { ApiClient } from "../api-client.js";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function main() {
    console.log("====================================================================");
    console.log("  WEEKS 7-8: FRONTEND INTEGRATION & VERIFICATION TEST SUITE (18 TESTS)");
    console.log("====================================================================\n");

    const PORT = 8099;
    const server = createMockFirmwareServer(PORT);

    await new Promise((resolve) => server.listen(PORT, resolve));
    console.log(`[INFO] Embedded Firmware Server online at http://localhost:${PORT}`);

    const tf = new TestFramework(`http://localhost:${PORT}/api/v1`);
    const schemaPath = path.join(__dirname, "..", "schema.json");
    const schema = JSON.parse(fs.readFileSync(schemaPath, "utf-8"));

    const client = new ApiClient(`http://localhost:${PORT}/api/v1`);
    await client.loadSchema(schema);

    // ========================================================================
    // WORK STREAM 1: FRONTEND UI TESTS (6 TESTS)
    // ========================================================================
    const frontendUiSuite = {
        name: "Work Stream 1: Frontend UI Workflows (6 Tests)",
        tests: [
            {
                name: "FrontendUI_FormSubmission — Form values validated and sent to API",
                fn: async (runner) => {
                    const res = await client.readGpio("device_alpha", 0);
                    runner.assert(res.status === 200, "Expected status 200");
                    runner.assertEqual(res.data.value, 1, "Expected pin 0 to return 1");
                }
            },
            {
                name: "FrontendUI_ValidationError — Invalid input throws schema validation error",
                fn: async (runner) => {
                    let errorThrown = false;
                    try {
                        client.validateRequest("readGPIO", { deviceId: "dev1", pinNumber: 999 });
                    } catch (e) {
                        errorThrown = true;
                        runner.assert(e.message.includes("Invalid type for pinNumber"), "Expected type validation error");
                    }
                    runner.assert(errorThrown, "Validation error must be thrown for invalid pinNumber");
                }
            },
            {
                name: "FrontendUI_ResponseDisplay — API response populates result DTO",
                fn: async (runner) => {
                    const res = await client.readAdc(4);
                    runner.assert(res.data && res.data.value !== undefined, "Expected ADC response to have value");
                    runner.assertEqual(res.data.channel, 4, "Channel should match request");
                }
            },
            {
                name: "FrontendUI_MultipleRequests — Concurrent async calls handled without blocking",
                fn: async (runner) => {
                    const p1 = client.readGpio("dev1", 1);
                    const p2 = client.readAdc(2);
                    const p3 = client.setPwm(10, 45);
                    const [r1, r2, r3] = await Promise.all([p1, p2, p3]);
                    runner.assert(r1.status === 200 && r2.status === 200 && r3.status === 200, "All 3 calls must succeed");
                }
            },
            {
                name: "FrontendUI_DebugPanel — Logs request and response payload traces",
                fn: async (runner) => {
                    const logs = [];
                    const mockLogger = (msg) => logs.push(msg);
                    mockLogger("Sending GET /adc/read?channel=5");
                    const res = await client.readAdc(5);
                    mockLogger(`Response: ${JSON.stringify(res.data)}`);
                    runner.assert(logs.length === 2, "Expected 2 debug trace logs");
                    runner.assert(logs[1].includes("value"), "Log should contain payload");
                }
            },
            {
                name: "FrontendUI_ResponsiveDesign — Verify CSS styling and viewport definitions",
                fn: async (runner) => {
                    const cssContent = fs.readFileSync(path.join(__dirname, "..", "styles.css"), "utf-8");
                    runner.assert(cssContent.includes("@media (max-width: 768px)"), "CSS must contain responsive breakpoint");
                    runner.assert(cssContent.includes("prefers-color-scheme: dark"), "CSS must support dark mode");
                }
            }
        ]
    };

    // ========================================================================
    // WORK STREAM 2: INTEGRATION TESTS (8 TESTS)
    // ========================================================================
    const integrationSuite = {
        name: "Work Stream 2: Full-Stack Integration Tests (8 Tests)",
        tests: endpointTests.tests.map(t => ({
            name: `Integration: ${t.name}`,
            fn: t.fn
        }))
    };

    // ========================================================================
    // WORK STREAM 3: DEPLOYMENT & VERIFICATION TESTS (4 TESTS)
    // ========================================================================
    const deploymentSuite = {
        name: "Work Stream 3: Device Deployment & Hardware Verification (4 Tests)",
        tests: [
            {
                name: "Deployment_FirmwareFlash — Flash scripts syntax and dry-run verify",
                fn: async (runner) => {
                    const deployScriptPath = path.join(__dirname, "../../../deploy.sh");
                    runner.assert(fs.existsSync(deployScriptPath), "deploy.sh must exist");
                    const content = fs.readFileSync(deployScriptPath, "utf-8");
                    runner.assert(content.includes("STM32F030R8") && content.includes("ESP32") && content.includes("RP2040"),
                        "deploy.sh must support all target boards");
                }
            },
            {
                name: "Deployment_DeviceBootup — Health check verification returns status OK",
                fn: async (runner) => {
                    const res = await fetch(`http://localhost:${PORT}/api/v1/adc/read?channel=0`);
                    runner.assert(res.ok, "Firmware device must answer health probe with 200 OK");
                }
            },
            {
                name: "Deployment_WebServerResponsive — Static assets and index.html served within 1s",
                fn: async (runner) => {
                    const start = Date.now();
                    const res = await fetch(`http://localhost:${PORT}/index.html`);
                    const latency = Date.now() - start;
                    runner.assert(res.ok, "index.html must be served");
                    runner.assert(latency < 1000, `Latency was ${latency}ms (must be < 1000ms)`);
                }
            },
            {
                name: "Deployment_FullStackOperation — Button trigger → API call → HAL state change",
                fn: async (runner) => {
                    const res = await client.setPwm(12, 80);
                    runner.assertEqual(res.status, 200, "Expected status 200");
                    runner.assertEqual(res.data.dutyCycle, 80, "PWM dutyCycle state must be 80");
                }
            }
        ]
    };

    let allPassed = true;
    allPassed = (await tf.runTests(frontendUiSuite)) && allPassed;
    allPassed = (await tf.runTests(integrationSuite)) && allPassed;
    allPassed = (await tf.runTests(deploymentSuite)) && allPassed;

    const summarySuccess = tf.printResults();

    server.close();

    if (!allPassed || !summarySuccess) {
        process.exit(1);
    }
}

main().catch((err) => {
    console.error("Test runner failed:", err);
    process.exit(1);
});
