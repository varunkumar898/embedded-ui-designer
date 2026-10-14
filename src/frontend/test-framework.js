// test-framework.js - Lightweight asynchronous integration test framework

class TestFramework {
    constructor(apiBaseUrl = "http://localhost:8080/api/v1") {
        this.apiBaseUrl = apiBaseUrl.replace(/\/+$/, "");
        this.results = [];
        this.currentTest = null;
    }
    
    async runTests(testSuite) {
        console.log(`\n============================================================`);
        console.log(`  RUNNING TEST SUITE: ${testSuite.name}`);
        console.log(`============================================================`);
        
        for (const test of testSuite.tests) {
            this.currentTest = test.name;
            try {
                await test.fn(this);
                this.pass(test.name);
            } catch (error) {
                this.fail(test.name, error.message);
            }
        }
        
        return this.results.filter(r => r.status === "FAIL").length === 0;
    }
    
    pass(testName) {
        this.results.push({ name: testName, status: "PASS", error: null });
        console.log(`  [PASS] ${testName}`);
    }
    
    fail(testName, error) {
        this.results.push({ name: testName, status: "FAIL", error: error });
        console.log(`  [FAIL] ${testName}: ${error}`);
    }
    
    assert(condition, message) {
        if (!condition) {
            throw new Error(message || "Assertion failed");
        }
    }
    
    assertEqual(actual, expected, message) {
        if (actual !== expected) {
            throw new Error(`${message || "Assertion failed"}: expected ${expected}, got ${actual}`);
        }
    }
    
    printResults() {
        console.log("\n" + "=".repeat(60));
        const passed = this.results.filter(r => r.status === "PASS").length;
        const total = this.results.length;
        console.log(`RESULTS: ${passed}/${total} tests passed (${Math.round((passed/total)*100)}%)`);
        console.log("=".repeat(60) + "\n");
        return passed === total;
    }
}

if (typeof module !== "undefined" && module.exports) {
    module.exports = { TestFramework };
}
export { TestFramework };
