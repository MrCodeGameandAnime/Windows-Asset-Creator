# Windows Asset Creator

Windows Asset Creator is a local Windows utility for turning one PNG or JPEG source image into a validated Microsoft Store/MSIX asset ZIP. It is a native C++20 WinUI 3 application: no Python runtime, .NET dependency, cloud processing, accounts, or telemetry are required.

## User workflow

1. Drag a PNG/JPEG onto the app, or choose one with **Browse**.
2. Review the grouped generated-asset board. Artwork is centered on a transparent square and is never cropped.
3. Select **Save As…**, choose a ZIP name and location, then File Explorer opens with the saved ZIP selected.

The ZIP contains the fixed Store/MSIX profile: 69 PNG assets under `Assets/` plus `AppIcon.ico`.

## Build prerequisites

The Visual Studio IDE is optional. Command-line builds require:

- Visual Studio Build Tools with the native C++ toolchain and MSBuild.
- The Windows application development / Windows App SDK build components for WinUI 3 C++.
- A Windows 10 or later SDK.
- Restored NuGet packages for the Windows App SDK, C++/WinRT, and WebView2 dependencies.

## Build and test

From the repository root:

```powershell
msbuild .\root\WindowsAssetCreator.sln /m /restore `
  /p:RestorePackagesConfig=true `
  /p:Configuration=Debug /p:Platform=x64

.\root\x64\Debug\AssetCoreTests.exe
```

The native test executable covers the profile, image pipeline, staging, ZIP export, output validation, and board-state behavior.
