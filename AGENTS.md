# Repository Guidelines

## Project Structure & Module Organization

This is an STM32F4 firmware project generated for STM32CubeMX and Keil MDK-ARM.

- `test.ioc` stores the CubeMX pin, clock, and middleware configuration.
- `Core/Inc` and `Core/Src` contain application entry points, interrupt handlers, HAL MSP setup, and generated initialization.
- `User/OLED` contains the SSD1306 OLED driver and fonts used by the application.
- `Drivers` contains STM32 HAL and CMSIS vendor libraries; avoid editing these unless updating the vendor package.
- `MDK-ARM` contains the Keil project, startup assembly, debug configuration, and build outputs.

Keep changes in generated files inside `/* USER CODE BEGIN ... */` / `/* USER CODE END ... */` blocks so CubeMX does not overwrite them.

## Build, Test, and Development Commands

- Open `MDK-ARM/test.uvprojx` in Keil uVision and build the target with **Project > Build target**.
- Use **Project > Rebuild all target files** after changing compiler options, startup files, or HAL configuration.
- Regenerate peripheral setup from `test.ioc` with STM32CubeMX when pin, clock, or middleware settings change, then rebuild in Keil.

There is no Makefile, CMake project, or command-line test runner currently configured.

## Coding Style & Naming Conventions

Use C with the existing STM32 HAL style. Follow local formatting: 4-space indentation in user code, same-line braces for compact helpers, and uppercase macros such as `OLED_ADDR`. Use HAL handle names consistently (`hi2c1`, `huart2`) and descriptive driver prefixes such as `ssd1306_` or `OLED_`.

Place public declarations in matching headers under `Core/Inc` or `User/<module>`, and keep private helpers `static` where possible.

## Testing Guidelines

No automated tests are present. Validate changes by building the Keil target and testing on STM32F411 hardware. For peripheral work, verify I2C/UART/GPIO behavior and document manual test steps in the pull request.

## Commit & Pull Request Guidelines

This repository has no existing commit history. Use short imperative commit messages, optionally scoped, for example `oled: fix display initialization`.

Pull requests should include a description, affected hardware/peripherals, build result, and manual test notes. Add screenshots or serial logs when UI or UART output changes. Mention CubeMX regeneration and avoid mixing vendor updates with application logic.

## Security & Configuration Tips

Do not commit local Keil preferences, temporary build artifacts, or machine-specific debug settings unless intentionally shared. Document board addresses, clock assumptions, and display dimensions near dependent code.
