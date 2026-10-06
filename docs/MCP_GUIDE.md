# Embedded UI Designer – Model Context Protocol (MCP) Guide

This guide details the **Model Context Protocol (MCP)** integration for **Embedded UI Designer**, enabling Claude, Antigravity, and other AI agents to programmatically inspect, design, manipulate, and export embedded user interfaces in real time.

---

## 1. Architectural Overview

The MCP integration enables live, two-way communication between AI assistants and the running Embedded UI Designer application without editing C++ source files or reloading the workspace.

```
+--------------------------------------------------------------------+
|                       Claude / Antigravity                        |
+--------------------------------------------------------------------+
                                  |
                                  | stdio (JSON-RPC 2.0)
                                  v
+--------------------------------------------------------------------+
|               MCP Server (mcp/embedded_ui_mcp.py)                  |
|    - Exposes 27 MCP Tools + 5 MCP Resources                        |
|    - Handles client handshakes, tool schemas, parameter validation |
+--------------------------------------------------------------------+
                                  |
                                  | Loopback TCP Socket (127.0.0.1:8765)
                                  v
+--------------------------------------------------------------------+
|         DesignerLocalServer (src/api/DesignerLocalServer.cpp)      |
|    - Embedded in Qt event loop (runs on main thread)               |
|    - Dispatches RPC requests to DesignerController                 |
+--------------------------------------------------------------------+
                                  |
                                  v
+--------------------------------------------------------------------+
|          DesignerController (src/api/DesignerController.cpp)       |
|    - ComponentSchemaRegistry (dynamic reflection & validation)     |
|    - Project & CanvasScene mutation engine                         |
|    - QUndoStack command dispatcher:                                |
|        * AddComponentCommand                                       |
|        * DeleteComponentCommand                                    |
|        * MoveComponentCommand                                      |
|        * ResizeComponentCommand                                    |
|        * PropertyChangeCommand                                     |
+--------------------------------------------------------------------+
                                  |
           +----------------------+----------------------+
           v                                             v
+---------------------+                       +----------------------+
|  Live CanvasScene   |                       |   PropertiesPanel    |
|   (Instant Redraw)  |                       |   & LayerPanel Sync  |
+---------------------+                       +----------------------+
```

### Key Principles
1. **Never edit source code for canvas changes**: AI commands execute directly against the running application's memory model.
2. **100% Undoable via `QUndoStack`**: All mutations (component creation, deletion, property edits, movement, resize) push standard Qt `QUndoCommand`s. You can always say *"Undo that"* or press `Ctrl+Z` in the app.
3. **Live Two-Way Synchronization**: Canvas, Properties Panel, and Layer Panel update instantaneously upon MCP mutation.

---

## 2. Component System & Schema

### Progress Bar Component (`ProgressBarComponent`)
A professional, modern embedded UI widget:
- **Visuals**: Horizontal/vertical track with dark rounded outer track (`#1c2028`), vibrant fill bar (`#10b981`), contrast border (`#2d3441`), customizable radius (default 8px).
- **Properties**:
  - `minimum`: Double/int lower bound (default `0`)
  - `maximum`: Double/int upper bound (default `100`)
  - `value`: Current progress value (respects min/max range)
  - `orientation`: `"Horizontal"` or `"Vertical"`
  - `trackColor`: Color of outer background track (hex string)
  - `barColor` / `fillColor`: Color of active progress fill (hex string)
  - `borderColor`: Color of outer frame border (hex string)
  - `borderWidth`: Border line thickness in pixels
  - `cornerRadius`: Corner curvature radius in pixels
  - `visible`: Boolean visibility flag

### Shape Component (`RectangleComponent`)
Modern dark rounded card container:
- **Visuals**: Dark background (`#1e222a`), subtle border (`#323846`), 8px radius.
- **Properties**: `fillColor`, `strokeColor`, `strokeWidth`, `cornerRadius`, `visible`.

### Machine-Readable Schema Registry (`ComponentSchemaRegistry`)
All 11 component types (`progress_bar`, `button`, `label`, `rectangle`, `slider`, `switch`, `checkbox`, `text_input`, `image`, `circle`, `path`) register their metadata and properties via `ComponentSchema.h`:
- Inspect via tool `list_component_types`
- Retrieve full JSON schema via tool `get_component_schema(type)`

---

## 3. Complete MCP Tool Reference

The MCP server provides **27 tools** across 7 functional categories:

### Project Management
| Tool | Parameters | Description |
|---|---|---|
| `get_project` | None | Returns project name, resolution, framework, and all component IDs |
| `get_project_metadata` | None | Returns project resolution (`width`, `height`), framework, and color depth |
| `create_project` | `name` (str), `width` (int), `height` (int), `framework` (str) | Creates a new blank project |
| `save_project` | `filePath` (str, optional) | Saves current project to `.euiproj` file |
| `load_project` | `filePath` (str) | Loads an existing `.euiproj` file |

### Component Discovery
| Tool | Parameters | Description |
|---|---|---|
| `list_component_types` | None | Lists all supported component types with human-readable titles |
| `get_component_schema` | `type` (str) | Returns property schema, defaults, min/max bounds, and options for a component type |
| `get_component` | `id` (str) | Returns full state (geometry, properties, z-order) of a specific component |
| `get_component_tree` | None | Returns hierarchical list of all canvas components |

### Component Creation & Deletion
| Tool | Parameters | Description |
|---|---|---|
| `create_component` | `type` (str), `x` (float, opt), `y` (float, opt), `width` (float, opt), `height` (float, opt), `properties` (dict, opt) | Instantiates and places a new component on canvas |
| `duplicate_component` | `id` (str) | Clones a component with an offset and creates an undoable command |
| `delete_component` | `id` (str) | Removes a component from canvas (undoable) |

### Component Editing & Mutation
| Tool | Parameters | Description |
|---|---|---|
| `set_component_property` | `id` (str), `property` (str), `value` (any) | Updates a single property through `PropertyChangeCommand` |
| `set_component_properties`| `id` (str), `properties` (dict) | Updates multiple properties simultaneously |
| `update_component` | `id` (str), `x`, `y`, `width`, `height`, `properties` | Updates geometry and properties in a single call |
| `move_component` | `id` (str), `x` (float), `y` (float) | Moves component to target coordinates (undoable) |
| `resize_component` | `id` (str), `width` (float), `height` (float) | Resizes component to target dimensions (undoable) |

### Selection Controls
| Tool | Parameters | Description |
|---|---|---|
| `get_selected_components` | None | Returns list of currently selected component IDs |
| `select_component` | `id` (str), `addToSelection` (bool, default false) | Selects a component on canvas and synchronizes the Properties Panel |
| `clear_selection` | None | Deselects all items on canvas |

### Canvas Inspection & State
| Tool | Parameters | Description |
|---|---|---|
| `get_canvas` | None | Returns complete canvas snapshot: resolution, background, and array of all component states |
| `get_canvas_size` | None | Returns `{ "width": int, "height": int }` of current target display |
| `clear_canvas` | None | Removes all components from canvas |

### History (Undo / Redo)
| Tool | Parameters | Description |
|---|---|---|
| `undo` | None | Undoes the last user or AI action on the canvas |
| `redo` | None | Redoes the last undone action |

### Code Generation & Export
| Tool | Parameters | Description |
|---|---|---|
| `export_project` | `outputDir` (str), `framework` (str: `lvgl`, `qul`, or `ugfx`) | Generates ready-to-flash C/C++/QML firmware project |
| `export_component` | `id` (str), `target` (str: `qml` or `ugfx`) | Emits snippet code for a single component |

---

## 4. MCP Resources

The MCP server exposes 5 read-only URI resources:
- `embedded-ui://project`: Current project metadata and component summary.
- `embedded-ui://canvas`: Real-time canvas hierarchy and all component geometries.
- `embedded-ui://components/types`: Complete list of supported widget types.
- `embedded-ui://selection`: Currently selected component details.
- `embedded-ui://schema`: Full JSON schema definitions for all component types.

---

## 5. Setup & Launch Instructions

### Step 1: Launch Embedded UI Designer
By default, the application automatically launches the local TCP control server on `127.0.0.1:8765`:

```bash
# Graphical mode:
./build_local/EmbeddedUIDesigner

# Custom MCP port:
./build_local/EmbeddedUIDesigner --mcp-port 9000

# Headless / CI mode:
QT_QPA_PLATFORM=offscreen ./build_local/EmbeddedUIDesigner --mcp-port 8765
```

### Step 2: Configure Claude Desktop
Edit your `claude_desktop_config.json`:
- **macOS**: `~/Library/Application Support/Claude/claude_desktop_config.json`
- **Linux**: `~/.config/Claude/claude_desktop_config.json`
- **Windows**: `%APPDATA%\Claude\claude_desktop_config.json`

Add the server:
```json
{
  "mcpServers": {
    "embedded-ui-designer": {
      "command": "python3",
      "args": [
        "/absolute/path/to/embedded-ui-designer/mcp/embedded_ui_mcp.py",
        "--port",
        "8765"
      ]
    }
  }
}
```

### Step 3: Configure Antigravity IDE
In Antigravity IDE or Gemini CLI, point to the provided `mcp/mcp_config.json` or register in `~/.gemini/antigravity-ide/mcp/embedded-ui-designer.json`:
```json
{
  "mcpServers": {
    "embedded-ui-designer": {
      "command": "python3",
      "args": [
        "/home/cherry/embedded-ui-designer/mcp/embedded_ui_mcp.py",
        "--port",
        "8765"
      ]
    }
  }
}
```

---

## 6. Example Natural-Language Commands & MCP Workflows

Once connected, Claude/Antigravity interprets natural language prompts and executes multi-step MCP workflows:

### Example 1: Creating a Dashboard
> **User**: "Create a 480x272 dark dashboard with a title label, a dark card container, and a green progress bar at the bottom."

**Claude Execution Plan**:
1. Calls `create_project(name="Dashboard", width=480, height=272)`
2. Calls `create_component(type="label", x=20, y=15, width=200, height=30, properties={"text": "SYSTEM METRICS", "textColor": "#ffffff", "pixelSize": 18, "bold": true})`
3. Calls `create_component(type="rectangle", x=20, y=60, width=440, height=140, properties={"fillColor": "#1e222a", "strokeColor": "#323846", "cornerRadius": 8})`
4. Calls `create_component(type="progress_bar", x=20, y=220, width=440, height=24, properties={"value": 65, "barColor": "#10b981", "trackColor": "#1c2028", "borderColor": "#2d3441", "cornerRadius": 6})`

### Example 2: Property Adjustments
> **User**: "Set the progress bar to 85% and change its fill to emerald green."

**Claude Execution Plan**:
1. Calls `get_canvas()` to find the progress bar's ID (`progress_bar_1`).
2. Calls `set_component_properties(id="progress_bar_1", properties={"value": 85, "barColor": "#059669"})`.
3. Application immediately updates canvas and Properties Panel via `PropertyChangeCommand`.

### Example 3: Reversible Changes
> **User**: "Make the card 20 pixels taller... actually, undo that."

**Claude Execution Plan**:
1. Calls `resize_component(id="rectangle_1", width=440, height=160)`.
2. Calls `undo()`. The card smoothly returns to 140px height.
