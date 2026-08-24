# Lumin ImGui Framework

Lumin is a C++20 Dear ImGui menu framework with a Direct3D 11 demo application. It includes a styled login flow, sidebar navigation, animated tab transitions, theme presets, glass overlays, toast notifications, and reusable widgets such as checkboxes, sliders, dropdowns, color pickers, keybinds, text fields, and search filters.

## Requirements

- Windows 10 or Windows 11
- Visual Studio 2022 with the Desktop development with C++ workload
- Windows 10/11 SDK installed through Visual Studio

ImGui, FreeType headers/library, stb_image, and UI font assets are vendored in this repository. The project uses Direct3D headers and libraries from the installed Windows SDK.

## Build

### Visual Studio

1. Open `framework.sln`.
2. Select `Release | x64`.
3. Build the solution.

The demo executable is generated at:

```text
thirdparty\imgui\examples\example_win32_directx11\Release\lumin_ui_demo.exe
```

### Command Line

From a Visual Studio Developer PowerShell:

```powershell
msbuild framework.sln /p:Configuration=Release /p:Platform=x64 /m
```

## Demo Login

The demo login accepts `demo` by default. This is only a sample UI gate, not an authentication or licensing system. Change `license_key_matches` in `framework/gui.cpp` before using the flow in a real product.

## Repository Layout

- `framework/` - Lumin UI framework code and assets.
- `thirdparty/imgui/` - Dear ImGui sources and backends.
- `thirdparty/freetype/` - FreeType headers and prebuilt Windows x64 library.
- `thirdparty/stb/` - stb_image single-header image loader.

## License

Original Lumin source and project files are licensed under the MIT License. Third-party components remain under their own licenses; see `THIRD_PARTY_NOTICES.md`.
