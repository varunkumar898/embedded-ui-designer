# Embedded UI Designer — Component Reference (Phase 2)

This document provides a comprehensive reference for all first-class visual and interactive components available in the **Embedded UI Designer**, including the Phase 2 component expansion.

---

## 1. Component Architecture & Hierarchy

All visual value components inherit from `ValueVisualizationComponent` (which extends `Component`), establishing a unified architecture for:
- Normalized value computation ($\text{value} \in [\min, \max]$ mapped to $[0.0, 1.0]$)
- Threshold management (Warning and Critical / Redline thresholds)
- State synchronization (`"normal"`, `"warning"`, `"critical"` / `"error"`, `"disabled"`, `"charging"`)
- Color styling (Track, Fill/Accent, Warning, Critical, Text colors)
- Phase 1 Data Source binding (`DataSource` $\rightarrow$ `Binding` $\rightarrow$ Component property)

```
Component (Base)
   │
   ├── ValueVisualizationComponent (Base for visual numeric readouts)
   │     ├── ProgressBarComponent (Linear Progress)
   │     ├── CircularProgressComponent (Radial Progress Ring)
   │     ├── GaugeComponent (Arc / Semi-Circle / Full-Circle Dial)
   │     ├── SpeedometerComponent (Automotive Speed Indicator)
   │     ├── BatteryComponent (Segmented / Fluid Battery Readout)
   │     ├── PressureComponent (Hydraulic / Pneumatic Dial)
   │     ├── RpmComponent (Engine Tachometer Readout)
   │     └── TemperatureComponent (Thermometer Gauge & Readout)
   │
   ├── Navigation / Container Components
   │     ├── TabViewComponent (Multi-page In-Component Switcher)
   │     └── NavigationBarComponent (Multi-Screen Navigation Bar)
   │
   └── Data Display Components
         ├── ListComponent (Embedded Dynamic Item List)
         └── TableComponent (2D Grid / Table View)
```

---

## 2. Component Categories in Palette

The designer organizes components into 5 clean categories:

| Category | Components | Purpose |
| :--- | :--- | :--- |
| **BASIC** | Button, Label, Shape (Rectangle, Circle, Path), Image | Core layout and static presentation primitives |
| **INPUT** | Switch, Checkbox, Slider, TextInput | Interactive controls and hardware-bound toggles |
| **DISPLAY** | Progress Bar, Circular Progress, Gauge, Speedometer, Battery, Pressure, RPM, Temperature | Visual value readouts, meters, dials, and telemetry gauges |
| **NAVIGATION** | Tab View, Navigation Bar | Multi-page and multi-screen navigation controls |
| **DATA** | List, Table | Embedded tabular and list data presentation |

---

## 3. Display & Telemetry Components (Phase 2)

### 3.1 Linear Progress (`ProgressBarComponent`)
- **Type Name**: `"progressbar"`
- **Description**: High-efficiency linear bar for displaying continuous levels (buffer, battery, download, fill level).
- **Key Properties**:
  - `value`: Current engineering value (e.g. `50.0`).
  - `minimum` / `maximum`: Range bounds (e.g. `0.0` to `100.0`).
  - `orientation`: `Horizontal` or `Vertical`.
  - `showValueText`: Toggle text display (`"50%"`, `"50.0"`).
  - `trackColor`, `fillColor`, `cornerRadius`, `borderWidth`, `borderColor`.
- **States**: `normal`, `warning`, `critical`, `disabled`.

---

### 3.2 Circular Progress (`CircularProgressComponent`)
- **Type Name**: `"circular_progress"`
- **Description**: Vector-rendered radial arc progress indicator with configurable stroke thickness and center readout.
- **Key Properties**:
  - `value`, `minimum`, `maximum`: Value bounds.
  - `startAngle`: Angle in degrees where arc starts (default: `225°`).
  - `sweepAngle`: Total angular range in degrees (default: `270°`).
  - `thickness`: Stroke width of the ring track and progress arc (default: `10px`).
  - `roundedCaps`: Smooth rounded cap rendering for high-DPI displays.
  - `showPercentage`: Displays centered percentage text (`"75%"`).
  - `centerLabel`: Optional auxiliary label (e.g., `"LOAD"`).

---

### 3.3 Configurable Gauge (`GaugeComponent`)
- **Type Name**: `"gauge"`
- **Description**: Industrial dial gauge supporting major/minor tick marks, label annotations, colored needle, and multiple presets.
- **Key Properties**:
  - `gaugeStyle`: `Arc` (default $270^\circ$), `SemiCircle` ($180^\circ$), or `FullCircle` ($360^\circ$).
  - `startAngle` / `sweepAngle`: Custom angle configurations.
  - `majorTicks` / `minorTicks`: Count of major numbered ticks and intermediate sub-ticks.
  - `showNeedle`: Needle pointer rendering with pivot circle.
  - `showLabels`: Numeric values at major tick locations.
  - `unit`: Display unit string (e.g., `"%"` , `"V"`, `"mA"`).
  - `trackColor`, `fillColor`, `needleColor`, `tickColor`.

---

### 3.4 Speedometer (`SpeedometerComponent`)
- **Type Name**: `"speedometer"`
- **Description**: Automotive-grade speedometer with warning and redline danger zones, needle indicator, and digital center readout.
- **Key Properties**:
  - `value`, `minimum`, `maximum`: Speed range (default `0` to `240`).
  - `speedUnit`: `"km/h"` or `"mph"`.
  - `speedometerStyle`: `Modern`, `Classic`, or `Compact`.
  - `warningThreshold`: Transition to warning arc (default `120.0`).
  - `criticalThreshold` / `redlineThreshold`: Danger/redline zone (default `180.0`).
  - `warningColor` (`#f59e0b`), `criticalColor` (`#ef4444`).

---

### 3.5 Battery Indicator (`BatteryComponent`)
- **Type Name**: `"battery"`
- **Description**: Battery state of charge indicator with terminal nub, segmented cell rendering, low/critical thresholds, and charging indicator.
- **Key Properties**:
  - `value`, `minimum`, `maximum`: Charge level (default `0` to `100`).
  - `orientation`: `Horizontal` or `Vertical`.
  - `isCharging`: Displays vector lightning charging glyph.
  - `cellCount`: Number of segmented visual bars (default `4`).
  - `warningThreshold` (`20.0`), `criticalThreshold` (`10.0`).
  - `showPercentage`: Shows text charge percentage (`"85%"`).

---

### 3.6 Pressure Indicator (`PressureComponent`)
- **Type Name**: `"pressure"`
- **Description**: Fluid and gas pressure meter for industrial HMI and telemetry displays.
- **Key Properties**:
  - `value`, `minimum`, `maximum`: Pressure range (default `0.0` to `10.0`).
  - `unit`: `"bar"`, `"psi"`, `"kPa"`, `"Pa"`.
  - `precision`: Number of fractional decimal digits (e.g. `1` for `4.2 bar`).
  - `warningThreshold` (`7.0 bar`), `criticalThreshold` (`8.5 bar`).
  - `trackColor`, `fillColor`, `warningColor`, `criticalColor`.

---

### 3.7 RPM Indicator (`RpmComponent`)
- **Type Name**: `"rpm"`
- **Description**: Engine and motor tachometer dial with $\times 1000$ tick multiplier, warning zone, and redline arc.
- **Key Properties**:
  - `value`, `minimum`, `maximum`: RPM range (default `0` to `8000`).
  - `warningThreshold`: Warning threshold (default `6000 RPM`).
  - `criticalThreshold`: Redline threshold (default `7000 RPM`).
  - `unit`: `"RPM"`.
  - `showNeedle`, `majorTicks`, `minorTicks`.

---

### 3.8 Temperature Indicator (`TemperatureComponent`)
- **Type Name**: `"temperature"`
- **Description**: Thermal monitor with liquid bulb/stem thermometer visual styling and digital readout.
- **Key Properties**:
  - `value`, `minimum`, `maximum`: Thermal range (default `-20°C` to `120°C`).
  - `tempUnit`: `"°C"` or `"°F"`.
  - `precision`: Decimal places (e.g. `72.5 °C`).
  - `warningThreshold` (`80.0 °C`), `criticalThreshold` (`100.0 °C`).
  - `showBulb`: Vector thermometer tube and bulb rendering.

---

## 4. Navigation & Container Components

### 4.1 Tab View (`TabViewComponent`)
- **Type Name**: `"tab_view"`
- **Description**: Self-contained multi-tab container allowing component-level page switching.
- **Key Properties**:
  - `tabs`: List of tab names (e.g. `["General", "Sensors", "Network"]`).
  - `activeTabIndex`: Index of currently active tab.
  - `tabPosition`: `Top` or `Bottom`.
  - `tabBarHeight`: Height allocated to tab headers.
  - `activeTabColor`, `inactiveTabColor`, `tabTextColor`.

---

### 4.2 Navigation Bar (`NavigationBarComponent`)
- **Type Name**: `"navigation_bar"`
- **Description**: Top/bottom screen navigation bar linking directly to project Screen IDs.
- **Key Properties**:
  - `items`: List of `NavItem` entries `{ label, icon, targetScreenId, enabled }`.
  - `activeIndex`: Currently highlighted navigation index.
  - `orientation`: `Horizontal` or `Vertical`.
  - `itemSpacing`: Padding between navigation tabs.
  - `activeColor`, `inactiveColor`.

---

## 5. Data Display Components

### 5.1 List (`ListComponent`)
- **Type Name**: `"list"`
- **Description**: Embedded scrolling item list with selection states and alternating row colors.
- **Key Properties**:
  - `items`: String list of rows.
  - `selectedIndex`: Currently selected row index.
  - `rowHeight`: Height per item (default `28px`).
  - `itemSpacing`: Space between rows.
  - `alternatingColors`: Alternating background contrast for rows.
  - `selectedColor`, `itemColor`, `textColor`.

---

### 5.2 Table (`TableComponent`)
- **Type Name**: `"table"`
- **Description**: 2D data table grid with header row, custom column names, and row selection.
- **Key Properties**:
  - `columns`: List of column headers (e.g. `["Channel", "Value", "Status"]`).
  - `rows`: 2D list of cell values.
  - `selectedRow`: Currently selected row index.
  - `headerHeight`: Header row height (default `28px`).
  - `rowHeight`: Data row height (default `24px`).
  - `showGrid`: Grid border visibility.
  - `headerColor`, `gridColor`, `selectedRowColor`.

---

## 6. Data Source Binding

Every visual component integrates with the Phase 1 Data Source system.

### Configuration in Properties Panel:
1. **Source**: Select bound source (`"None"`, or custom Data Source ID like `"sensor_temp"`, `"vehicle_speed"`, `"adc_channel_0"`).
2. **Property**: Bound component property (`"value"`, `"color"`, `"state"`, `"text"`).
3. **Direction**: `Read` (Component receives updates from Data Source), `Write` (Component sends user input to Data Source), or `ReadWrite`.

---

## 7. Component State System

Components implement clean state transitions:
- **`normal`**: Standard visual palette.
- **`warning`**: Amber/orange accent (`#f59e0b`).
- **`critical` / `error`**: Red accent (`#ef4444`).
- **`charging`**: Green/cyan battery active fill.
- **`disabled`**: Muted low-contrast opacity (`#64748b`).

State changes are automatically evaluated by threshold rules or can be explicitly driven by data bindings.
