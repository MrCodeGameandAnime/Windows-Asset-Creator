# Contributing to Windows Asset Creator

Thanks for considering a contribution. Windows Asset Creator (WAC) is a native Windows utility that creates a fixed set of MSIX assets from one PNG or JPEG. Contributions should preserve that focused purpose and the existing generation and export contracts unless a change is specifically intended to revise them.

## Before you start

- For a bug, include the WAC version or commit, Windows version, steps to reproduce, and the result you expected and observed. Remove personal paths or other private data from logs and screenshots before sharing them.
- For a proposed behavior change, open an issue or discussion first so the scope can be agreed before substantial work begins.
- Do not report a suspected security vulnerability in a public issue. Follow [SECURITY.md](SECURITY.md).

## Development requirements

Development and verification require Windows with Visual Studio 2022 or its Build Tools, MSBuild, the **Desktop development with C++** workload, a Windows SDK, and the Windows App SDK / WinUI 3 build dependencies used by the solution. The Visual Studio IDE is optional; command-line MSBuild is the documented build path.

## Build and test

From the repository root, run the same Debug x64 build used by CI:

```powershell
msbuild root\WindowsAssetCreator.sln /m /restore /p:RestorePackagesConfig=true /p:Configuration=Debug /p:Platform=x64
```

Then run the native test executable:

```powershell
.\root\x64\Debug\AssetCoreTests.exe
```

If your change affects the WinUI presentation, package behavior, or file intake/export flow, also describe any runtime verification performed and include a concise reproduction path for anything that could not be exercised locally.

## Pull requests

- Keep each change focused and explain its user-visible effect or engineering reason.
- Add or update focused native tests when behavior in the core or state layer changes.
- Run the build and native tests above, and report the results in the pull request.
- For UI changes, include before/after screenshots when they help reviewers assess the result.
- Update relevant documentation when behavior, setup, or supported inputs change.
- Do not include build outputs, package artifacts, temporary files, logs, caches, IDE metadata, signing certificates, private keys, or secrets unless a file is explicitly required by the source contract.
- Be respectful and constructive in project spaces; contributions are subject to the [Code of Conduct](CODE_OF_CONDUCT.md).

## Licensing

By intentionally submitting a contribution for inclusion, you agree that it is provided under the terms of the project’s [Apache License 2.0](../LICENSE.md), unless a separate written agreement applies.
