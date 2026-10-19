# Phase G1 report — custom board/pack functions (inspection only, no code changes)

Status: read-only inspection performed on 2026-10-19 on branch
`feat/pin-plan-codegen` (tip `01a4973`). No code was changed by Phase G1.

## What exists today

1. **Data model** — `src/driverpack/CustomPackModel.h/.cpp`. The custom pack JSON
   schema contains: `device_identity {name, vendor, part_number}`, `family`,
   `core`, `clock_speed_mhz` (a single scalar), `memory_regions[{name, type,
   start_address, size_kb, usable_for_framebuffer}]`,
   `peripherals[{name, base_address, type, description,
   registers[{name, offset, description, reset_value}]}]`,
   `pin_table[{pin, function, peripheral}]` (one function row per pin),
   `board_wiring {display_interface, display_pins, touch_pins, power_rails}`,
   `sdk_reference {path, required_files, detected}`, `toolchain`,
   `templates_used`,
   `provenance {source_files, importer_version, generated_by (user|importer|ai),
   verified (defaults to false)}`.
   The validator (`CustomPackValidator`) checks required identity/family/core,
   hex-address syntax, that pin rows reference existing peripherals, and
   provenance. It does not detect function conflicts between pin rows.
2. **SVD importer** — `src/driverpack/CmsisSvdImporter.h/.cpp`. 10 MB file-size
   cap, XML depth-64 cap, and explicit rejection of external-entity DTDs
   (XXE protection), all covered by tests. It parses device name/vendor, CPU
   core name and `clockFrequency`, and peripherals + registers (name, base
   address, register offsets, reset values). It does **not** parse
   `<interrupt>` elements (IRQ names/numbers), memory regions, or register
   arrays/`derivedFrom`. If `groupName` is missing, the peripheral type is
   inferred from the name prefix (GPIO/USART-uart/SPI/I2C); anything else
   falls back to the generic `"PERIPHERAL"` type.
3. **Pack manager** — `src/driverpack/CustomPackManager.h/.cpp`. On install: it
   validates, writes `<AppData>/custom_packs/<name>.json`, adds a `DeviceProfile`
   to the `CapabilityDatabase` (with `verified = false` and sources
   `"imported pack: <name>"`), and registers a `CustomDriverPack` in
   `DriverPackRegistry`. On load: it re-reads every `*.json` in the directory and
   skips invalid packs. Two issues were found here (see gaps below).
4. **Generator** — `src/driverpack/CustomDriverPack.cpp` (`generateProjectTree`)
   produces six artifacts: `<name>_registers.h` (peripheral base and register
   access macros), `pin_config.h` (**string-only macros** of the shape
   `#define PIN_<pin>_FUNC "<function>"`), `target_hal.c/.h` stubs (empty bodies
   with TODO comments, one variant per SDK-detected / raw-register mode), a
   small `main.c`, and a flat `Makefile`. `compilationReady` is set to `true`
   unconditionally (no compile check runs). On export,
   `FirmwareExportManager` writes only `generated/target/<name>_registers.h`
   from this project; the C1 plan-driven generation never runs for custom
   devices (its gate is STM32F030R8-specific).
5. **Import UI** — `src/dialogs/CustomPackImportDialog.h/.cpp`. It already
   supports: browsing/importing an SVD (via the SVD importer), importing a **CSV
   pin table** (`PinTableParser::parseCsvFile`), and scanning/verifying an SDK
   directory (`SdkReferenceChecker`), plus a manual pin-table widget.
6. **Tests** — `tests/test_custom_packs.cpp` (suite `CustomPacksAcceptanceTests`,
   tests): valid/invalid pack creation and validation, SVD import (valid + XXE
   + oversized file + deep nesting), pin table parsing (valid + unknown
   peripheral rejection), SDK reference checking, pack installation round-trip
   through the manager into the `CapabilityDatabase` and registry, export from a
   custom pack (clean output with the registers header), and the
   unsupported-device export no-op.

## Gaps relative to the Phase G goals (custom MCU/board designer, family-independent drivers)

1. **Pins**: the pin table has no alternate-function/mux value field (it must be
   expressible as `null` when unknown), no per-pin list of functions (only one
   row per pin, and repeated rows for the same pin are not detected as
   conflicts), no function-conflict validation, and no GPIO
   output-level register mapping (only string macros today).
2. **Interrupts**: the SVD importer does not parse `<interrupt>`, and the data
   model has no IRQ table at all, so per-peripheral IRQ names/numbers cannot be
   captured yet.
3. **Protocols**: no CAN modeling (no `kind` field in the data model, so no
   CAN-specific bit-timing inputs and no CAN code), no single-wire/1-Wire pads,
   no ADC channel-on-pin mapping, no per-instance constraints (baud ranges,
   FIFO depth), and no clock tree beyond the single global
   `clock_speed_mhz` scalar — without which CAN/UART bit timing cannot be
   computed from sourced facts.
4. **Generator output**: currently standing-by interfaces only; there is no
   family-independent `hal_*.h` interface, no configuration tables, no
   user hooks, and no `board.h`-style named pins/buses. The generated
   `target_hal` uses only string macros, the device header name is derived from
   the pack name instead of cross-checking the SVD device name, the generated
   `Makefile` has no cross-toolchain script path or linker script, and
   `compilationReady = true` is set unconditionally without any compile check
   having run.
5. **Provenance completeness**: the pack has `source_files` and
   `importer_version`, but no per-input-file hashes, no license field, and no
   schema-version/kind markers (unlike the vendor-facing capability packs under
   `data/hardware_packs`, which carry `schemaVersion`/`kind`/`provenance`
   with explicit licenses). The same provenance fields should be aligned for
   custom packs.
6. **Manager limits**: `installPack` writes the pack name verbatim into a
   filename — unsafe against path controllers (`"a/../b"`, absolute paths,
   separator characters, Windows-reserved names such as `CON`, and overlong
   names are all accepted today). `uninstallPack` deletes the file and the
   internal map entry but ignores the `CapabilityDatabase` argument (the device
   profile is not removed) and does not unregister the pack from
   `DriverPackRegistry`. Reload can register duplicate packs
   (`DriverPackRegistry::registerPack` simply appends).
7. **Export integration**: the custom-device path ignores `PeripheralPlan` —
   the wizard's pin/peripheral selection never reaches generation for custom
   devices. No link between the C1 machinery and `CustomDriverPack`.

## Proposed custom-device data model (v2, backward-compatible with v1 loaders)

```jsonc
{
  "schemaVersion": "2.0",
  "kind": "customDevice",
  "deviceIdentity": {
    "name", "vendor", "partNumber", "package",
    "toolchain", "sdkFlavours": [
      "stm32-cube-hal/ll", "esp-idf", "pico-sdk", "zephyr",
      "mcuxpresso", "register-level-svd"
    ]
  },
  "clockTree": {
    "sources": [{ "name", "type": "hsi|hse|pll|internal-rc|user", "frequencyHz" }],
    "buses": [{ "name", "domain", "maxFrequencyHz": null }]  // null → Info
  , "systemClockHz": null                                    // null → Info
  },
  "peripherals": [{
    "name": "USART1", "kind": "UART|SPI|I2C|GPIO|CAN|ONEWIRE|ADC|TIMER|RAW",
    "baseAddress": null | "0x…",     // facts from the user or SVD; null → Info
    "irq": { "name", "number" } | null,
    "registers": [ /* same field set as v1 (unchanged) */ ],
    "constraints": { "maxBaud": null, "fifoDepth": null }      // null → Info
  }],
  "pins": [{
    "pin": "PA0",
    "functions": [{
      "fn": "USART1_TX|GPIO|ADC_IN|…",
      "af": null | integer | name,   // null = unknown, never invented
      "adcChannel": null | integer
    }],
    "analogOk": true | false | null
  }],
  "interrupts": [{ "name", "number", "source" }],
  "board": {
    "display": { /* keeps existing structure */ },
    "buttons": [{ "pin", "label" }],
    "leds": [{ "pin", "label" }],
    "debugProbeType": "swd|jtag|manufacturer-uart|null",
    "crystal": { "lowSpeedHz": null, "highSpeedHz": null }
  },
  "provenance": {
    "sourceFiles": [{ "path", "sha256" }],
    "importerVersion", "license": null,       // null → Info, never assumed
    "verified": false,                        // ALWAYS false by default
    "generatedBy": "user|importer|ai",
    "customNote": "Custom (user-defined, unverified)"
  }
}
```

Rules for the v2 model: every missing or unknown field stays `null` and produces
an Info diagnostic; conflicts produce Error diagnostics; no fact is invented.
Peripheral kind, clock, IRQ, pins/AF, CAN/ADC constraints, and interrupts all
cross-check against Svd data as soon as the SVD parse is extended with those
fields.

## Proposed G2 work plan (next step, gated on review)

1. v2 schema + migration (existing v1 files keep loading; an explicit
   migration note is shown, no silent probing).
2. Extend the SVD importer to parse `<interrupt>` and memory regions and to
   record per-input file hashes plus license fields.
3. Dialog extension: pin-table editor with per-pin function lists (respecting
   the v2 schema), clock tree tab, IRQ table tab, peripherals list tab, and a
   side-by-side preview of the board definition.
4. Keep the existing `verified = false` invariant, golden-file rules, and
   "no inventing" coding constraints.
