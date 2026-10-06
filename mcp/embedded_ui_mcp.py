#!/usr/bin/env python3
"""
Embedded UI Designer — Model Context Protocol (MCP) Server
Enables Claude and Antigravity to discover, inspect, create, style, move,
and manipulate components in the running Embedded UI Designer live application.

Standard JSON-RPC 2.0 stdio server communicating over localhost TCP to the
running C++ Qt EmbeddedUIDesigner application.
"""

import sys
import json
import socket
import argparse
import logging

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s", stream=sys.stderr)
logger = logging.getLogger("EmbeddedUIMCP")

DEFAULT_PORT = 8765

TOOLS_DEFINITION = [
    # ── PROJECT TOOLS ──
    {
        "name": "get_project",
        "description": "Get current active project metadata, canvas dimensions, target framework, dirty state, color styles, and all components.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "get_project_metadata",
        "description": "Get lightweight metadata about the currently opened project.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "create_project",
        "description": "Create a new embedded UI project with the given name and resolution dimensions.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string", "description": "Project name (e.g. 'ThermostatUI')"},
                "width": {"type": "integer", "description": "Display width in pixels (e.g. 480)"},
                "height": {"type": "integer", "description": "Display height in pixels (e.g. 272)"}
            },
            "required": ["name", "width", "height"]
        }
    },
    {
        "name": "save_project",
        "description": "Save current project to its active file or a specified .euiproj path.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "filePath": {"type": "string", "description": "Optional destination file path ending in .euiproj"}
            }
        }
    },
    {
        "name": "load_project",
        "description": "Load an existing .euiproj project file into the running designer.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "filePath": {"type": "string", "description": "Path to .euiproj project file"}
            },
            "required": ["filePath"]
        }
    },

    # ── COMPONENT DISCOVERY TOOLS ──
    {
        "name": "list_component_types",
        "description": "List all registered component types (progress_bar, button, label, rectangle, slider, switch, checkbox, text_input, image, circle, path) with categories.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "get_component_schema",
        "description": "Get machine-readable property schema for a component type or all types, including property names, types, defaults, and ranges.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "type": {"type": "string", "description": "Component type (e.g. 'progress_bar', 'button', 'rectangle', or empty for all)"}
            }
        }
    },
    {
        "name": "get_component",
        "description": "Get complete state of a specific component on the canvas by its ID, including geometry and all customizable properties.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Component unique ID (e.g. 'progress_bar_1', 'btn_start')"}
            },
            "required": ["id"]
        }
    },
    {
        "name": "get_component_tree",
        "description": "Get a lightweight summary tree of all components currently placed on the canvas.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },

    # ── COMPONENT CREATION & DELETION ──
    {
        "name": "create_component",
        "description": "Create a new component on the canvas with optional geometry and properties. Undoable via QUndoStack.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "type": {"type": "string", "description": "Component type: 'progress_bar', 'button', 'label', 'rectangle', 'slider', 'switch', 'checkbox', 'text_input', 'image', 'circle', 'path'"},
                "x": {"type": "number", "description": "X coordinate in canvas pixels"},
                "y": {"type": "number", "description": "Y coordinate in canvas pixels"},
                "width": {"type": "number", "description": "Optional width in pixels (uses default if omitted)"},
                "height": {"type": "number", "description": "Optional height in pixels (uses default if omitted)"},
                "id": {"type": "string", "description": "Optional stable custom ID (auto-generated if omitted)"},
                "properties": {"type": "object", "description": "Component-specific property key/value pairs"}
            },
            "required": ["type", "x", "y"]
        }
    },
    {
        "name": "duplicate_component",
        "description": "Duplicate an existing component with an offset. Undoable.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Source component ID"},
                "dx": {"type": "number", "description": "Horizontal offset in pixels (default: 20)"},
                "dy": {"type": "number", "description": "Vertical offset in pixels (default: 20)"}
            },
            "required": ["id"]
        }
    },
    {
        "name": "delete_component",
        "description": "Delete a component from the canvas. Undoable via QUndoStack.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Component ID to remove"}
            },
            "required": ["id"]
        }
    },

    # ── COMPONENT EDITING ──
    {
        "name": "update_component",
        "description": "Batch update geometry and properties of an existing component.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Target component ID"},
                "x": {"type": "number", "description": "Optional new X position"},
                "y": {"type": "number", "description": "Optional new Y position"},
                "width": {"type": "number", "description": "Optional new width"},
                "height": {"type": "number", "description": "Optional new height"},
                "visible": {"type": "boolean", "description": "Optional visibility flag"},
                "properties": {"type": "object", "description": "Dictionary of properties to update"}
            },
            "required": ["id"]
        }
    },
    {
        "name": "set_component_property",
        "description": "Set a single property on a component (e.g. value, barColor, cornerRadius, text, minimum, maximum). Undoable.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Target component ID (e.g. 'progress_bar_1')"},
                "property": {"type": "string", "description": "Property name (e.g. 'value', 'barColor', 'cornerRadius', 'text', 'minimum', 'maximum')"},
                "value": {"description": "New property value"}
            },
            "required": ["id", "property", "value"]
        }
    },
    {
        "name": "set_component_properties",
        "description": "Set multiple properties on a component atomically in a single undoable step.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Target component ID"},
                "properties": {"type": "object", "description": "Key/value dictionary of properties to update"}
            },
            "required": ["id", "properties"]
        }
    },
    {
        "name": "move_component",
        "description": "Move a component to a new absolute position or by a relative delta. Undoable.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Target component ID"},
                "x": {"type": "number", "description": "Absolute X coordinate"},
                "y": {"type": "number", "description": "Absolute Y coordinate"},
                "dx": {"type": "number", "description": "Relative horizontal delta"},
                "dy": {"type": "number", "description": "Relative vertical delta"}
            },
            "required": ["id"]
        }
    },
    {
        "name": "resize_component",
        "description": "Resize a component to new width and height in canvas pixels. Undoable.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Target component ID"},
                "width": {"type": "number", "description": "New width (> 0)"},
                "height": {"type": "number", "description": "New height (> 0)"}
            },
            "required": ["id", "width", "height"]
        }
    },

    # ── SELECTION TOOLS ──
    {
        "name": "get_selected_components",
        "description": "Get a list of currently selected components on the canvas.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "select_component",
        "description": "Select a component on the canvas and activate it in the PropertiesPanel inspector.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Component ID to select"},
                "addToSelection": {"type": "boolean", "description": "If true, add to existing multi-selection without clearing"}
            },
            "required": ["id"]
        }
    },
    {
        "name": "clear_selection",
        "description": "Deselect all components on the canvas.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },

    # ── CANVAS TOOLS ──
    {
        "name": "get_canvas",
        "description": "Get full canvas layout, display resolution, background color, and every component with bounds and properties.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "get_canvas_size",
        "description": "Get current canvas width and height in pixels.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "clear_canvas",
        "description": "Remove all components from the canvas. Undoable.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },

    # ── HISTORY TOOLS ──
    {
        "name": "undo",
        "description": "Undo the last user or AI action via QUndoStack.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "redo",
        "description": "Redo the previously undone action via QUndoStack.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },

    # ── EXPORT TOOLS ──
    {
        "name": "export_project",
        "description": "Generate embedded code for the full project using LVGL, Qt Quick Ultralite (QUL), or µGFX.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "target": {"type": "string", "description": "Target framework: 'lvgl', 'qul', or 'ugfx'"},
                "outputDir": {"type": "string", "description": "Destination directory for generated C/C++/QML code"}
            },
            "required": ["target"]
        }
    },
    {
        "name": "export_component",
        "description": "Export single component C/QML code snippet.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Component ID"},
                "target": {"type": "string", "description": "'qml' or 'ugfx'"}
            },
            "required": ["id"]
        }
    },

    # ── UNIVERSAL HARDWARE & PINMUX TOOLS (PHASES 22, 23, 24) ──
    {
        "name": "get_target",
        "description": "Get current active hardware target (MCU, Board, or Custom) configured in the project, including vendor, family, architecture, core, flash, RAM, and counts.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "set_target",
        "description": "Select and configure an MCU or development board target for the active project.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "targetType": {"type": "string", "enum": ["device", "board", "custom"], "description": "'device' (MCU) or 'board'"},
                "id": {"type": "string", "description": "Device partNumber (e.g. 'STM32F407VG', 'ESP32-S3', 'RP2040') or Board ID (e.g. 'STM32F407G-DISC1', 'ESP32-S3-DevKitC-1', 'Raspberry Pi Pico')"}
            },
            "required": ["id"]
        }
    },
    {
        "name": "list_devices",
        "description": "Discover supported microcontrollers and microprocessors with optional filtering by vendor, family, or keyword search.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "vendor": {"type": "string", "description": "Filter by vendor (e.g. 'STMicroelectronics', 'Espressif', 'Raspberry Pi')"},
                "family": {"type": "string", "description": "Filter by family (e.g. 'STM32', 'ESP32', 'RP2040')"},
                "search": {"type": "string", "description": "Keyword search string (part number, core, architecture)"}
            }
        }
    },
    {
        "name": "list_boards",
        "description": "Discover supported development boards with physical headers, connectors, and onboard peripherals.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "vendor": {"type": "string", "description": "Filter by manufacturer"},
                "mcu": {"type": "string", "description": "Filter by MCU part number"},
                "search": {"type": "string", "description": "Search board name, family, or architecture"}
            }
        }
    },
    {
        "name": "get_device_info",
        "description": "Retrieve comprehensive authoritative datasheet specifications for a microcontroller (pins, alternate functions, peripherals, clock, package, memory).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "deviceId": {"type": "string", "description": "Part number, e.g. 'STM32F407VG', 'ESP32-S3', 'RP2040'"}
            },
            "required": ["deviceId"]
        }
    },
    {
        "name": "get_board_info",
        "description": "Retrieve comprehensive board specifications including physical headers, pin labels, power rails, LEDs, buttons, and debug interfaces.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "boardId": {"type": "string", "description": "Board identifier, e.g. 'STM32F407G-DISC1', 'ESP32-S3-DevKitC-1', 'Raspberry Pi Pico'"}
            },
            "required": ["boardId"]
        }
    },
    {
        "name": "get_pinout",
        "description": "Retrieve complete physical pinout with alternate functions and package positions for a device or current project target.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "deviceId": {"type": "string", "description": "Optional device ID. If omitted, uses active project device."}
            }
        }
    },
    {
        "name": "list_pins",
        "description": "List all physical pins for the active project target with their current configuration status (mode, label, pull, speed, alternate function, assigned).",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "get_pin",
        "description": "Inspect single pin configuration, alternate function options, and conflict state on the active target.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "pin": {"type": "string", "description": "Pin name (e.g. 'PA5', 'GPIO4', 'GP16')"}
            },
            "required": ["pin"]
        }
    },
    {
        "name": "configure_pin",
        "description": "Configure GPIO/alternate function properties for a physical pin with authoritative conflict detection.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "pin": {"type": "string", "description": "Pin identifier (e.g. 'PA5', 'PB6', 'GPIO4')"},
                "config": {
                    "type": "object",
                    "properties": {
                        "mode": {"type": "string", "description": "'GPIO_Input', 'GPIO_Output', 'Analog', 'AlternateFunction', 'None'"},
                        "label": {"type": "string", "description": "Custom functional label, e.g. 'STATUS_LED' or 'TOUCH_CS'"},
                        "alternateFunction": {"type": "string", "description": "Alternate function name, e.g. 'SPI1_SCK', 'I2C1_SCL'"},
                        "pull": {"type": "string", "enum": ["NoPull", "PullUp", "PullDown"], "description": "Internal pull resistor"},
                        "speed": {"type": "string", "enum": ["Low", "Medium", "High", "VeryHigh"], "description": "Slew rate / output speed"},
                        "outputType": {"type": "string", "enum": ["PushPull", "OpenDrain"], "description": "Output stage"},
                        "initialOutput": {"type": "string", "enum": ["Low", "High"], "description": "Initial state"},
                        "interrupt": {"type": "string", "enum": ["None", "RisingEdge", "FallingEdge", "BothEdges"], "description": "Interrupt trigger"}
                    }
                },
                "force": {"type": "boolean", "description": "Force assignment, replacing conflicting owner if already assigned"}
            },
            "required": ["pin", "config"]
        }
    },
    {
        "name": "configure_pins",
        "description": "Batch configure multiple physical pins at once, validating pinmux constraints.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "assignments": {
                    "type": "array",
                    "description": "List of pin configuration objects (pin, mode, label, alternateFunction, pull, speed)",
                    "items": {"type": "object"}
                },
                "force": {"type": "boolean", "description": "Force assignment if conflicts occur"}
            },
            "required": ["assignments"]
        }
    },
    {
        "name": "list_peripherals",
        "description": "List all on-chip communication and control peripherals (UART, SPI, I2C, CAN, ADC, Timers, USB, etc.) for the current target with enabled status.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "get_peripheral",
        "description": "Retrieve peripheral configuration, supported signals, and mapped pins for a specific peripheral (e.g. 'USART1', 'SPI1', 'I2C1').",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string", "description": "Peripheral name (e.g. 'USART1', 'SPI1', 'I2C1')"}
            },
            "required": ["name"]
        }
    },
    {
        "name": "configure_peripheral",
        "description": "Configure peripheral communication parameters (baudRate, clockSpeedHz, mode) and map peripheral signals to pins.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string", "description": "Peripheral name (e.g. 'USART1', 'SPI1', 'I2C1')"},
                "config": {
                    "type": "object",
                    "properties": {
                        "enabled": {"type": "boolean", "description": "Enable or disable this peripheral"},
                        "type": {"type": "string", "description": "Peripheral type (e.g. 'USART', 'SPI', 'I2C')"},
                        "pins": {"type": "object", "description": "Signal to pin mappings, e.g. {'TX': 'PA9', 'RX': 'PA10'}"},
                        "parameters": {"type": "object", "description": "Bus parameters, e.g. {'baudRate': 115200, 'dataBits': 8}"}
                    }
                },
                "force": {"type": "boolean", "description": "Force pin reallocation if pins are occupied"}
            },
            "required": ["name", "config"]
        }
    },
    {
        "name": "get_available_pins",
        "description": "Query which pins are capable of serving a given peripheral signal on the active microcontroller (e.g. peripheral 'SPI1', signal 'SCK').",
        "inputSchema": {
            "type": "object",
            "properties": {
                "peripheral": {"type": "string", "description": "Peripheral name (e.g. 'SPI1', 'USART1')"},
                "signal": {"type": "string", "description": "Signal name (e.g. 'SCK', 'MOSI', 'TX', 'SCL')"}
            },
            "required": ["peripheral", "signal"]
        }
    },
    {
        "name": "get_free_pins",
        "description": "Get list of all general-purpose I/O pins that are currently unassigned and available for components or peripherals.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "validate_hardware_configuration",
        "description": "Run pinmux validation engine to detect conflicts, reserved/power pin violations, duplicate peripherals, or missing required pins.",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "get_hardware_configuration",
        "description": "Get complete active hardware configuration JSON (target, pins, peripherals, clock, toolchain).",
        "inputSchema": {
            "type": "object",
            "properties": {}
        }
    },
    {
        "name": "create_custom_hardware",
        "description": "Register a new custom MCU or Board definition in the database.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "definition": {"type": "object", "description": "Device or Board JSON definition"}
            },
            "required": ["definition"]
        }
    },
    {
        "name": "save_hardware_definition",
        "description": "Export a hardware device or board definition to an external JSON pack file.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Target ID or part number"},
                "filePath": {"type": "string", "description": "Destination file path ending in .json"}
            },
            "required": ["id", "filePath"]
        }
    },
    {
        "name": "load_hardware_definition",
        "description": "Import an external hardware definition JSON file into the database.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "filePath": {"type": "string", "description": "Path to hardware definition JSON file"}
            },
            "required": ["filePath"]
        }
    }
]

RESOURCES_DEFINITION = [
    {
        "uri": "project://current",
        "name": "Current Embedded Project",
        "description": "Complete state of the active embedded project, metadata, and components",
        "mimeType": "application/json"
    },
    {
        "uri": "hardware://target",
        "name": "Active Hardware Target",
        "description": "Current active MCU, board, architecture, memory, and pin counts",
        "mimeType": "application/json"
    },
    {
        "uri": "hardware://pins",
        "name": "Hardware Pinout State",
        "description": "Current pin assignments, alternate functions, and available GPIOs",
        "mimeType": "application/json"
    },
    {
        "uri": "hardware://peripherals",
        "name": "Hardware Peripherals State",
        "description": "Configured communications interfaces (UART, SPI, I2C, CAN, etc.)",
        "mimeType": "application/json"
    },
    {
        "uri": "canvas://current",
        "name": "Current Canvas Layout",
        "description": "Resolution, background color, and visual elements currently on the canvas",
        "mimeType": "application/json"
    },
    {
        "uri": "selection://current",
        "name": "Current Selection",
        "description": "List of currently selected components in the designer",
        "mimeType": "application/json"
    },
    {
        "uri": "schema://components",
        "name": "Component Schema Registry",
        "description": "Machine-readable schema describing every component and customizable property",
        "mimeType": "application/json"
    },
    {
        "uri": "types://components",
        "name": "Component Types",
        "description": "List of available component types and categories",
        "mimeType": "application/json"
    }
]


class DesignerClient:
    def __init__(self, host="127.0.0.1", port=DEFAULT_PORT):
        self.host = host
        self.port = port

    def send_command(self, method: str, params: dict = None) -> dict:
        if params is None:
            params = {}
        payload = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": method,
            "params": params
        }
        msg = json.dumps(payload) + "\n"

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(5.0)
                s.connect((self.host, self.port))
                s.sendall(msg.encode("utf-8"))

                # Read until newline
                buf = b""
                while True:
                    chunk = s.recv(4096)
                    if not chunk:
                        break
                    buf += chunk
                    if b"\n" in buf:
                        break

                line = buf.decode("utf-8").strip()
                if not line:
                    return {"success": False, "error": "Empty response from Embedded UI Designer"}
                resp = json.loads(line)
                if "error" in resp:
                    return {"success": False, "error": resp["error"].get("message", "Unknown error"), "data": resp["error"]}
                return resp.get("result", {"success": True})
        except ConnectionRefusedError:
            return {
                "success": False,
                "error": f"Cannot connect to Embedded UI Designer at {self.host}:{self.port}. Please ensure the application is running."
            }
        except socket.timeout:
            return {"success": False, "error": "Connection timed out communicating with Embedded UI Designer"}
        except Exception as e:
            return {"success": False, "error": f"Socket communication error: {str(e)}"}


def handle_request(req: dict, client: DesignerClient) -> dict:
    req_id = req.get("id")
    method = req.get("method")
    params = req.get("params", {})

    if method == "initialize":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "protocolVersion": "2024-11-05",
                "capabilities": {
                    "tools": {},
                    "resources": {}
                },
                "serverInfo": {
                    "name": "embedded-ui-designer-mcp",
                    "version": "1.0.0"
                }
            }
        }

    if method == "notifications/initialized":
        return None  # Notification, no response needed

    if method == "ping":
        return {"jsonrpc": "2.0", "id": req_id, "result": {}}

    if method == "tools/list":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "tools": TOOLS_DEFINITION
            }
        }

    if method == "tools/call":
        tool_name = params.get("name")
        args = params.get("arguments", {})
        result = client.send_command(tool_name, args)

        is_error = not result.get("success", True)
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "content": [
                    {
                        "type": "text",
                        "text": json.dumps(result, indent=2)
                    }
                ],
                "isError": is_error
            }
        }

    if method == "resources/list":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "resources": RESOURCES_DEFINITION
            }
        }

    if method == "resources/read":
        uri = params.get("uri", "")
        data = {}
        if uri == "project://current":
            data = client.send_command("get_project")
        elif uri == "canvas://current":
            data = client.send_command("get_canvas")
        elif uri == "selection://current":
            data = client.send_command("get_selected_components")
        elif uri == "schema://components":
            data = client.send_command("get_component_schema")
        elif uri == "types://components":
            data = client.send_command("list_component_types")
        elif uri == "hardware://target":
            data = client.send_command("get_target")
        elif uri == "hardware://pins":
            data = client.send_command("list_pins")
        elif uri == "hardware://peripherals":
            data = client.send_command("list_peripherals")
        else:
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "error": {
                    "code": -32602,
                    "message": f"Resource not found: {uri}"
                }
            }

        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "contents": [
                    {
                        "uri": uri,
                        "mimeType": "application/json",
                        "text": json.dumps(data, indent=2)
                    }
                ]
            }
        }

    return {
        "jsonrpc": "2.0",
        "id": req_id,
        "error": {
            "code": -32601,
            "message": f"Method not found: {method}"
        }
    }


def main():
    parser = argparse.ArgumentParser(description="Embedded UI Designer MCP Server")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Port where Embedded UI Designer is listening")
    parser.add_argument("--host", type=str, default="127.0.0.1", help="Host address of Embedded UI Designer")
    args = parser.parse_args()

    client = DesignerClient(host=args.host, port=args.port)
    logger.info("Embedded UI Designer MCP Server initialized (connecting to %s:%d)", args.host, args.port)

    # Process stdin JSON-RPC requests line by line
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            req = json.loads(line)
            resp = handle_request(req, client)
            if resp is not None:
                sys.stdout.write(json.dumps(resp) + "\n")
                sys.stdout.flush()
        except json.JSONDecodeError as e:
            err_resp = {
                "jsonrpc": "2.0",
                "id": None,
                "error": {"code": -32700, "message": f"Parse error: {str(e)}"}
            }
            sys.stdout.write(json.dumps(err_resp) + "\n")
            sys.stdout.flush()
        except Exception as e:
            logger.error("Unexpected error handling request: %s", str(e), exc_info=True)
            err_resp = {
                "jsonrpc": "2.0",
                "id": None,
                "error": {"code": -32603, "message": f"Internal error: {str(e)}"}
            }
            sys.stdout.write(json.dumps(err_resp) + "\n")
            sys.stdout.flush()


if __name__ == "__main__":
    main()
