#include "DocumentationGenerator.h"
#include <sstream>
#include <fstream>

namespace documentation {

DocumentationGenerator::DocumentationGenerator() {}

std::string DocumentationGenerator::generateApiDocumentation(
    const std::vector<ApiEndpointDoc>& endpoints,
    DocumentFormat format) {
    
    std::stringstream ss;
    ss << "# REST API Documentation\n\n";
    ss << "Auto-generated from ProjectModel schema.\n\n";
    
    // Group by tag
    std::map<std::string, std::vector<const ApiEndpointDoc*>> byTag;
    for (const auto& ep : endpoints) {
        if (ep.tags.empty()) {
            byTag["General"].push_back(&ep);
        } else {
            for (const auto& tag : ep.tags) {
                byTag[tag].push_back(&ep);
            }
        }
    }
    
    for (const auto& [tag, tagEndpoints] : byTag) {
        ss << "## " << tag << "\n\n";
        
        for (const auto* ep : tagEndpoints) {
            ss << toMarkdown(*ep);
            ss << "\n\n";
        }
    }
    
    std::string markdown = ss.str();
    if (format == DocumentFormat::HTML) {
        return toHtml(markdown);
    }
    return markdown;
}

std::string DocumentationGenerator::generateTypeDocumentation(
    const std::vector<TypeDoc>& types,
    DocumentFormat format) {
    
    std::stringstream ss;
    ss << "# Type Definitions\n\n";
    ss << "All data types used in the REST API.\n\n";
    
    for (const auto& type : types) {
        ss << toMarkdown(type);
        ss << "\n\n";
    }
    
    std::string markdown = ss.str();
    if (format == DocumentFormat::HTML) {
        return toHtml(markdown);
    }
    return markdown;
}

std::string DocumentationGenerator::generateCompatibilityMatrix(
    const std::vector<HardwareCompatibility>& boards,
    DocumentFormat format) {
    
    std::stringstream ss;
    ss << "# Hardware Compatibility Matrix\n\n";
    ss << "| Board | Architecture | GPIO | ADC | PWM | UART | SPI | I2C | Full Support |\n";
    ss << "|-------|--------------|------|-----|-----|------|-----|-----|-------|\n";
    
    for (const auto& board : boards) {
        ss << "| " << board.boardName << " | " << board.architecture;
        
        for (const auto& feature : board.supportedFeatures) {
            (void)feature;
            ss << " | ✓";
        }
        
        for (size_t i = board.supportedFeatures.size(); i < 6; ++i) {
            ss << " | ✗";
        }
        
        ss << " | " << (board.fullSupport ? "✓" : "✗") << " |\n";
    }
    
    std::string markdown = ss.str();
    if (format == DocumentFormat::HTML) {
        return toHtml(markdown);
    }
    return markdown;
}

std::string DocumentationGenerator::generateIntegrationGuide(
    const std::vector<ApiEndpointDoc>& endpoints,
    DocumentFormat format) {
    (void)endpoints;
    std::stringstream ss;
    ss << "# Integration Guide for Hardware Manufacturers\n\n";
    ss << "Step-by-step instructions for integrating the embedded UI framework into your board.\n\n";
    
    ss << "## 1. Hardware Setup\n";
    ss << "- Connect GPIO pins according to your board's pin layout\n";
    ss << "- Configure ADC channels for sensor input\n";
    ss << "- Enable PWM timers for output control\n\n";
    
    ss << "## 2. Firmware Build\n";
    ss << "```bash\n";
    ss << "cd firmware && make BOARD=STM32F030R8\n";
    ss << "make flash DEVICE=/dev/ttyUSB0\n";
    ss << "```\n\n";
    
    ss << "## 3. REST API Testing\n";
    ss << "```bash\n";
    ss << "curl -X GET http://192.168.1.100:8080/api/v1/readGpio?pin=5\n";
    ss << "```\n\n";
    
    ss << "## 4. Web Dashboard Access\n";
    ss << "Open http://192.168.1.100:8080 in your browser.\n\n";
    
    std::string markdown = ss.str();
    if (format == DocumentFormat::HTML) {
        return toHtml(markdown);
    }
    return markdown;
}

std::string DocumentationGenerator::generateTroubleshootingGuide(
    DocumentFormat format) {
    
    std::stringstream ss;
    ss << "# Troubleshooting Guide\n\n";
    
    ss << "## Issue: Device not responding to HTTP requests\n";
    ss << "**Solution:**\n";
    ss << "1. Check device is powered and connected to network\n";
    ss << "2. Verify IP address: ping 192.168.1.100\n";
    ss << "3. Check firewall: ensure port 8080 is open\n";
    ss << "4. Restart embedded web server: power cycle the device\n\n";
    
    ss << "## Issue: Firmware flashing fails\n";
    ss << "**Solution:**\n";
    ss << "1. Install board-specific driver (OpenOCD for STM32, esptool.py for ESP32)\n";
    ss << "2. Check USB cable connection\n";
    ss << "3. Verify device enters bootloader mode\n";
    ss << "4. Try: openocd -f stm32f0.cfg -c 'program firmware.elf verify reset'\n\n";
    
    ss << "## Issue: GPIO read returns incorrect value\n";
    ss << "**Solution:**\n";
    ss << "1. Verify pin configuration in ProjectModel\n";
    ss << "2. Check electrical connections (pull-ups, pull-downs)\n";
    ss << "3. Inspect HAL implementation for board\n";
    ss << "4. Review audit logs for auth/validation errors\n\n";
    
    std::string markdown = ss.str();
    if (format == DocumentFormat::HTML) {
        return toHtml(markdown);
    }
    return markdown;
}

std::string DocumentationGenerator::generateCodeExamples(
    const std::vector<ApiEndpointDoc>& endpoints,
    const std::string& language) {
    (void)endpoints;
    std::stringstream ss;
    ss << "# Code Examples (" << language << ")\n\n";
    
    if (language == "Python") {
        ss << "```python\n";
        ss << "import requests\n";
        ss << "import json\n\n";
        ss << "BASE_URL = 'http://192.168.1.100:8080/api/v1'\n";
        ss << "API_KEY = 'your-api-key-here'\n\n";
        
        ss << "headers = {'Authorization': f'ApiKey {API_KEY}'}\n\n";
        
        ss << "# Read GPIO pin\n";
        ss << "response = requests.get(f'{BASE_URL}/readGpio?pin=5', headers=headers)\n";
        ss << "print(response.json())\n\n";
        
        ss << "# Set PWM\n";
        ss << "data = {'pin': 3, 'dutyCycle': 50}\n";
        ss << "response = requests.post(f'{BASE_URL}/setPwm', json=data, headers=headers)\n";
        ss << "print(response.status_code)\n";
        ss << "```\n\n";
    } else if (language == "JavaScript") {
        ss << "```javascript\n";
        ss << "const BASE_URL = 'http://192.168.1.100:8080/api/v1';\n";
        ss << "const API_KEY = 'your-api-key-here';\n\n";
        
        ss << "const headers = { 'Authorization': `ApiKey ${API_KEY}` };\n\n";
        
        ss << "// Read GPIO pin\n";
        ss << "fetch(`${BASE_URL}/readGpio?pin=5`, { headers })\n";
        ss << "  .then(r => r.json())\n";
        ss << "  .then(data => console.log(data));\n\n";
        
        ss << "// Set PWM\n";
        ss << "fetch(`${BASE_URL}/setPwm`, {\n";
        ss << "  method: 'POST',\n";
        ss << "  headers: { ...headers, 'Content-Type': 'application/json' },\n";
        ss << "  body: JSON.stringify({ pin: 3, dutyCycle: 50 })\n";
        ss << "})\n";
        ss << ".then(r => console.log(r.status));\n";
        ss << "```\n\n";
    } else if (language == "C++") {
        ss << "```cpp\n";
        ss << "#include <iostream>\n";
        ss << "#include <string>\n\n";
        ss << "int main() {\n";
        ss << "    std::cout << \"Sending readGpio request to MCU\" << std::endl;\n";
        ss << "    return 0;\n";
        ss << "}\n";
        ss << "```\n\n";
    } else if (language == "C") {
        ss << "```c\n";
        ss << "#include <stdio.h>\n\n";
        ss << "int main(void) {\n";
        ss << "    printf(\"Reading GPIO pin on STM32\\n\");\n";
        ss << "    return 0;\n";
        ss << "}\n";
        ss << "```\n\n";
    } else if (language == "Rust") {
        ss << "```rust\n";
        ss << "fn main() {\n";
        ss << "    println!(\"Embedded HAL GPIO control\");\n";
        ss << "}\n";
        ss << "```\n\n";
    }
    
    return ss.str();
}

bool DocumentationGenerator::exportToFile(const std::string& content, 
                                         const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;
    
    file << content;
    return file.good();
}

std::string DocumentationGenerator::toMarkdown(const ApiEndpointDoc& endpoint) {
    std::stringstream ss;
    ss << "### " << endpoint.operationId << "\n\n";
    ss << "**" << endpoint.method << " " << endpoint.path << "**\n\n";
    ss << endpoint.description << "\n\n";
    
    if (!endpoint.parameters.empty()) {
        ss << "**Parameters:**\n";
        for (const auto& [param, desc] : endpoint.parameters) {
            ss << "- `" << param << "`: " << desc << "\n";
        }
        ss << "\n";
    }
    
    ss << "**Response:**\n";
    ss << "```json\n" << endpoint.responseSchema << "\n```\n";
    
    if (!endpoint.examples.empty()) {
        ss << "\n**Examples:**\n";
        for (const auto& example : endpoint.examples) {
            ss << "```\n" << example << "\n```\n";
        }
    }
    
    return ss.str();
}

std::string DocumentationGenerator::toMarkdown(const TypeDoc& type) {
    std::stringstream ss;
    ss << "### " << type.typeName << "\n\n";
    ss << type.description << "\n\n";
    ss << "**Type ID:** `" << type.typeId << "`\n";
    ss << "**Base Type:** `" << type.baseType << "`\n\n";
    
    if (!type.fields.empty()) {
        ss << "**Fields:**\n";
        ss << "| Field | Type |\n";
        ss << "|-------|------|\n";
        for (const auto& [field, fieldType] : type.fields) {
            ss << "| " << field << " | " << fieldType << " |\n";
        }
        ss << "\n";
    }
    
    return ss.str();
}

std::string DocumentationGenerator::toHtml(const std::string& markdown) {
    std::stringstream ss;
    ss << "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"><title>Documentation</title></head><body>\n";
    ss << "<pre>\n" << markdown << "\n</pre>\n";
    ss << "</body></html>\n";
    return ss.str();
}

} // namespace documentation
