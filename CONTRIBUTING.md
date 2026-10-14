# Contributing to Embedded UI Designer Platform

Thank you for your interest in contributing to the **Embedded UI Designer Platform**! We welcome contributions, bug reports, feature requests, and pull requests from developers of all skill levels.

## Development Workflow

1. **Fork the Repository**:
   Fork the repository to your own GitHub account and clone it locally:
   ```bash
   git clone https://github.com/<your-username>/embedded-ui-designer.git
   cd embedded-ui-designer
   ```

2. **Set Up Development Environment**:
   Run the setup script for your platform:
   - **Linux (x86_64)**: `./scripts/setup-linux-x86.sh`
   - **Linux (ARM64 / Raspberry Pi)**: `./scripts/setup-linux-arm.sh`
   - **macOS (Intel)**: `./scripts/setup-macos-x86.sh`
   - **macOS (Apple Silicon)**: `./scripts/setup-macos-arm.sh`
   - **Windows (x86_64 PowerShell)**: `.\scripts\setup-windows-x86.ps1`
   - **Windows (Cross-ARM PowerShell)**: `.\scripts\setup-windows-cross-arm.ps1`

3. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

4. **Code Standards**:
   - **C++**: Follow C++17 standards with clean abstraction, RAII, and zero compiler warnings.
   - **C**: Clean, MISRA-compliant C99 for bare-metal firmware modules.
   - **Rust**: Format with `cargo fmt` and verify with `cargo clippy`.
   - **JavaScript / Frontend**: Modern ES6+ modules without heavy bundler dependencies.

5. **Testing & Validation**:
   Ensure all 102 acceptance tests pass before opening a Pull Request:
   ```bash
   mkdir -p build && cd build
   cmake -DENABLE_TESTS=ON ..
   make -j$(nproc)
   ctest --verbose
   ```
   Or run the individual test phases:
   ```bash
   ./build/test_project_model
   ./build/test_codegen_multi_language
   node ../src/frontend/tests/integration_runner.js
   ./build/test_security_performance
   ./build/test_compliance_observability
   ```

6. **Submit a Pull Request**:
   - Push your branch to GitHub: `git push -u origin feature/your-feature-name`
   - Open a PR against `main`.
   - Ensure GitHub Actions CI passes across all 5 platforms.
