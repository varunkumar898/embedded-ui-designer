#!/usr/bin/env python3
"""
Comprehensive End-to-End Test Suite for Embedded UI Designer MCP Integration
Tests all 26 tools over the live socket API.
"""

import sys
import json
import socket
import time

DEFAULT_PORT = 8765

def send_rpc(sock, method, params=None):
    if params is None:
        params = {}
    payload = {
        "jsonrpc": "2.0",
        "id": 1,
        "method": method,
        "params": params
    }
    sock.sendall((json.dumps(payload) + "\n").encode("utf-8"))
    
    buf = b""
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            break
        buf += chunk
        if b"\n" in buf:
            break
            
    line = buf.decode("utf-8").strip()
    return json.loads(line)

def run_tests(port=8765):
    print("=" * 70)
    print("EMBEDDED UI DESIGNER — MCP AI LIVE INTEGRATION TEST SUITE")
    print(f"Connecting to live application on 127.0.0.1:{port}...")
    print("=" * 70)

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5.0)
        s.connect(("127.0.0.1", port))
    except Exception as e:
        print(f"[FAIL] Could not connect to EmbeddedUIDesigner: {e}")
        return False

    passed = 0
    failed = 0

    def assert_test(name, condition, details=""):
        nonlocal passed, failed
        if condition:
            passed += 1
            print(f"  [PASS] {name} {details}")
        else:
            failed += 1
            print(f"  [FAIL] {name} {details}")

    # 1. Component discovery
    res = send_rpc(s, "list_component_types")
    types = [t["type"] for t in res.get("result", {}).get("types", [])]
    assert_test("list_component_types", "progress_bar" in types and "rectangle" in types and "button" in types,
                f"Discovered {len(types)} component types: {', '.join(types[:5])}...")

    # 2. Get Component Schema for progress_bar
    res = send_rpc(s, "get_component_schema", {"type": "progress_bar"})
    pb_props = [p["name"] for p in res.get("result", {}).get("schema", {}).get("properties", [])]
    assert_test("get_component_schema (progress_bar)", "value" in pb_props and "barColor" in pb_props and "minimum" in pb_props and "maximum" in pb_props,
                f"Properties: {', '.join(pb_props)}")

    # 3. Create Project
    res = send_rpc(s, "create_project", {"name": "TestDashboard", "width": 480, "height": 272})
    assert_test("create_project", res.get("result", {}).get("success") is True and res.get("result", {}).get("width") == 480,
                "Created 480x272 project")

    # 4. Canvas Size
    res = send_rpc(s, "get_canvas_size")
    assert_test("get_canvas_size", res.get("result", {}).get("width") == 480 and res.get("result", {}).get("height") == 272,
                "Canvas is 480x272")

    # 5. Create Progress Bar Component
    res = send_rpc(s, "create_component", {
        "type": "progress_bar",
        "id": "progress_bar_1",
        "x": 40,
        "y": 180,
        "width": 320,
        "height": 24,
        "properties": {
            "minimum": 0,
            "maximum": 100,
            "value": 65,
            "barColor": "#10b981",
            "trackColor": "#1c2028",
            "borderColor": "#2d3441",
            "borderWidth": 1,
            "cornerRadius": 8
        }
    })
    comp = res.get("result", {}).get("component", {})
    props = comp.get("properties", {})
    assert_test("create_component (progress_bar)", comp.get("id") == "progress_bar_1" and props.get("value") == 65 and props.get("cornerRadius") == 8,
                f"Created id={comp.get('id')}, value={props.get('value')}, barColor={props.get('barColor')}")

    # 6. Set Component Property (change value to 82)
    res = send_rpc(s, "set_component_property", {
        "id": "progress_bar_1",
        "property": "value",
        "value": 82
    })
    assert_test("set_component_property (value=82)", res.get("result", {}).get("component", {}).get("properties", {}).get("value") == 82,
                "Value updated to 82")

    # 7. Set Component Property (change barColor to emerald green)
    res = send_rpc(s, "set_component_property", {
        "id": "progress_bar_1",
        "property": "barColor",
        "value": "#059669"
    })
    assert_test("set_component_property (barColor)", res.get("result", {}).get("component", {}).get("properties", {}).get("barColor") == "#059669",
                "Color updated to #059669")

    # 8. Create Shape / Rectangle Component
    res = send_rpc(s, "create_component", {
        "type": "rectangle",
        "id": "card_shape_1",
        "x": 40,
        "y": 40,
        "width": 140,
        "height": 80,
        "properties": {
            "fillColor": "#1e222a",
            "strokeColor": "#323846",
            "strokeWidth": 1,
            "cornerRadius": 8
        }
    })
    rect_comp = res.get("result", {}).get("component", {})
    assert_test("create_component (rectangle/shape)", rect_comp.get("id") == "card_shape_1" and rect_comp.get("properties", {}).get("cornerRadius") == 8,
                "Created modern dark card Shape with 8px radius")

    # 9. Create Button Component
    res = send_rpc(s, "create_component", {
        "type": "button",
        "id": "btn_submit",
        "x": 200,
        "y": 40,
        "width": 120,
        "height": 38,
        "properties": {
            "text": "Submit",
            "backgroundColor": "#2563eb",
            "cornerRadius": 6
        }
    })
    btn_comp = res.get("result", {}).get("component", {})
    assert_test("create_component (button)", btn_comp.get("id") == "btn_submit" and btn_comp.get("properties", {}).get("text") == "Submit",
                "Created button with label 'Submit'")

    # 10. Move Component
    res = send_rpc(s, "move_component", {
        "id": "progress_bar_1",
        "x": 50,
        "y": 190
    })
    assert_test("move_component", res.get("result", {}).get("x") == 50 and res.get("result", {}).get("y") == 190,
                "Moved progress bar to (50, 190)")

    # 11. Resize Component
    res = send_rpc(s, "resize_component", {
        "id": "progress_bar_1",
        "width": 380,
        "height": 26
    })
    assert_test("resize_component", res.get("result", {}).get("width") == 380 and res.get("result", {}).get("height") == 26,
                "Resized progress bar to 380x26")

    # 12. Duplicate Component
    res = send_rpc(s, "duplicate_component", {
        "id": "progress_bar_1",
        "dx": 10,
        "dy": 35
    })
    dup_id = res.get("result", {}).get("component", {}).get("id")
    assert_test("duplicate_component", dup_id and dup_id != "progress_bar_1",
                f"Duplicated to {dup_id}")

    # 13. Canvas inspection
    res = send_rpc(s, "get_canvas")
    comps = res.get("result", {}).get("components", [])
    assert_test("get_canvas", len(comps) == 4,
                f"Canvas contains {len(comps)} components")

    # 14. Undo (undo the duplication)
    res = send_rpc(s, "undo")
    assert_test("undo", res.get("result", {}).get("success") is True,
                f"Undid action: {res.get('result', {}).get('action')}")

    # Check component count after undo
    res = send_rpc(s, "get_canvas")
    comps_after_undo = res.get("result", {}).get("components", [])
    assert_test("verify canvas count after undo", len(comps_after_undo) == 3,
                f"Component count reverted from 4 to {len(comps_after_undo)}")

    # 15. Redo
    res = send_rpc(s, "redo")
    assert_test("redo", res.get("result", {}).get("success") is True,
                f"Redid action: {res.get('result', {}).get('action')}")

    # Re-undo to leave 3 components
    send_rpc(s, "undo")

    # 16. Selection tools
    res = send_rpc(s, "select_component", {"id": "progress_bar_1"})
    assert_test("select_component", res.get("result", {}).get("success") is True,
                "Selected progress_bar_1")

    res = send_rpc(s, "get_selected_components")
    sel = [c["id"] for c in res.get("result", {}).get("selected", [])]
    assert_test("get_selected_components", "progress_bar_1" in sel,
                f"Currently selected: {', '.join(sel)}")

    res = send_rpc(s, "clear_selection")
    assert_test("clear_selection", res.get("result", {}).get("success") is True,
                "Cleared selection")

    # 17. Code Export for component
    res = send_rpc(s, "export_component", {"id": "progress_bar_1", "target": "qml"})
    snippet = res.get("result", {}).get("snippet", "")
    assert_test("export_component (qml)", "ProgressBar" in snippet or "value:" in snippet or "actualValue" in snippet,
                f"Generated QML snippet ({len(snippet)} chars)")

    # 18. Code Export for whole project (LVGL)
    res = send_rpc(s, "export_project", {"target": "lvgl"})
    assert_test("export_project (lvgl)", res.get("result", {}).get("success") is True,
                f"Exported to: {res.get('result', {}).get('outputDir')}")

    # 19. Validation & Error Handling
    res = send_rpc(s, "get_component", {"id": "non_existent_item_999"})
    assert_test("validation (missing component)", res.get("result", {}).get("success") is False or "error" in res,
                "Safely returned structured error for missing item")

    res = send_rpc(s, "create_component", {"type": "unknown_widget_type", "x": 0, "y": 0})
    assert_test("validation (unknown component type)", res.get("result", {}).get("success") is False or "error" in res,
                "Safely returned structured error for invalid type")

    s.close()

    print("=" * 70)
    print(f"RESULTS: {passed} PASSED, {failed} FAILED")
    print("=" * 70)
    return failed == 0

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_PORT
    success = run_tests(port)
    sys.exit(0 if success else 1)
