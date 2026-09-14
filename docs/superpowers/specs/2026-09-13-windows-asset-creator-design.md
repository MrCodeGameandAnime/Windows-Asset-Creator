# Windows Asset Creator — MVP Design

## Purpose

Windows Asset Creator is a local, Windows-first utility that turns one master raster image into a validated Microsoft Store/MSIX asset package. It is a native C++20 application with a WinUI 3/C++/WinRT interface. It has no embedded Python runtime, .NET dependency, account system, telemetry, cloud processing, or image-editor features.

The application prioritizes package correctness and a short, familiar workflow:

1. The user drops an image onto the app, or chooses it with Explorer.
2. The app processes the image and displays the generated output in a scrollable asset board.
3. The user selects **Save As**, chooses a ZIP filename and location in the standard Windows picker, and the app opens File Explorer with the saved ZIP selected.

Generation occurs in a temporary session before Save As. Canceling Save As creates no user-visible output files. The app never deletes or overwrites a user-selected folder.

## Repository layout

The existing layout is retained and given concrete responsibilities:

```text
root/
  windows/                  WinUI 3 application; XAML views, view models, drag/drop,
                            preview board, Windows file picker, Explorer reveal
  src/                      UI-independent C++20 generator core
    Output/                 PNG, ICO, ZIP, staging, and validation output components
  tests/                    All automated tests and test fixtures
docs/
  superpowers/specs/        Approved technical design documents
```

`root/windows` may depend on `root/src`; `root/src` must not depend on WinUI, XAML, or application-window types. All test code belongs under `root/tests`.

## Architecture

The application has two layers.

### WinUI application (`root/windows`)

The packaged WinUI 3/C++/WinRT app owns interaction and presentation only:

- accepts dropped files and launches the image browse picker;
- exposes loading, ready, validation-failed, save-in-progress, saved, and error UI states;
- displays the source details and a vertically scrollable, grouped board of generated thumbnails;
- invokes the standard Windows Save As picker only after generation and validation succeed;
- opens File Explorer with the saved ZIP selected;
- runs core generation off the UI thread and returns UI updates through the dispatcher.

The asset board follows the approved **Asset board** layout: source summary at the top, grouped sections for AppList and logo families, ICO summary, validation summary, and a persistent Save As action.

### Generator core (`root/src`)

The core exposes a small session-oriented API with structured results and diagnostics. It is responsible for:

- decoding WIC-supported PNG and JPEG sources;
- applying EXIF orientation when present;
- converting every accepted source to a 32-bit RGBA WIC bitmap;
- normalizing non-square input by centering contained artwork on a transparent square, never cropping it;
- materializing the profile's render plan with WIC scaling and PNG encoding;
- creating the multi-entry ICO file with a small native ICO writer;
- validating the generated images and package plan;
- staging all files in a private temporary session;
- creating a ZIP from a validated session when the caller provides a Save As destination.

The core uses Windows Imaging Component for decode, pixel conversion, scaling, and PNG encoding. Image-processing implementation details remain behind narrow interfaces so they can be directly tested without a UI.

## Internal output profile

The MVP contains one internal, fixed `StoreMsixProfile`. It is deliberately a data table rather than logic scattered throughout the renderer. The UI does not expose a profile selector.

Each asset specification records:

- logical group and display label;
- output-relative filename;
- required pixel dimensions;
- PNG or ICO format;
- qualifier kind (`base`, `scale`, `targetsize`, or alternate form);
- validation requirements.

The current profile emits this package structure:

```text
<chosen-name>.zip
  Assets/
    AppList.targetsize-*.png
    AppList.targetsize-*_altform-unplated.png
    AppList.targetsize-*_altform-lightunplated.png
    Square44x44Logo.png and .scale-*.png variants
    Square150x150Logo.png and .scale-*.png variants
    StoreLogo.png and .scale-*.png variants
    MedTile.scale-*.png variants
  AppIcon.ico
```

`AppIcon.ico` replaces the reference script's product-specific `HeadsUp.ico`. No product-specific canonical master image is added to the ZIP; the ZIP contains only packaging assets.

The plan contains 69 PNG assets and one ICO:

| Group | Files | Sizes |
| --- | ---: | --- |
| AppList default | 14 | target sizes 16, 20, 24, 30, 32, 36, 40, 48, 60, 64, 72, 80, 96, 256 |
| AppList unplated / light-unplated | 28 | same target sizes |
| Square44x44Logo | 8 | base 44; scale 100/125/150/200/250/300/400 = 44/55/66/88/110/132/176 |
| Square150x150Logo | 8 | base 150; scale 100/125/150/200/250/300/400 = 150/188/225/300/375/450/600 |
| StoreLogo | 6 | base 50; scale 100/125/150/200/400 = 50/63/75/100/200 |
| MedTile | 5 | scale 100/125/150/200/400 = 150/188/225/300/600 |
| AppIcon.ico | 1 | 16, 24, 32, 48, and 256 entries |

All theme variants are generated from the same master image in the MVP. The app validates structural correctness; it does not claim to measure visual contrast or automatically redesign an icon for light and dark backgrounds.

## Rendering, validation, and error handling

The normalized master is always square RGBA. A rectangular source is placed on a transparent square whose side equals the longest source edge, centered without crop. Each planned image is resized with a high-quality WIC scaler and encoded as PNG.

Validation runs against the staged output, not only in-memory metadata. It verifies:

- every planned file exists exactly once;
- PNGs decode through WIC;
- output dimensions match the profile;
- PNG pixels use an alpha-capable 32-bit format before encoding;
- each ICO directory entry has an expected size and valid embedded image payload;
- ZIP contents match the validated staging manifest exactly.

Errors are returned as structured diagnostics with an operation, severity, code, user-facing message, and technical detail. Unsupported formats, corrupt images, WIC failures, inability to write the temporary session, Save As failures, and ZIP failures result in a clear UI state without partial saved output.

## Save As and temporary files

After a successful session is ready, Save As uses the Windows file-save picker with a product-neutral suggested name such as `Windows-Assets.zip`. The user may rename it and choose any writable location. The core writes the ZIP to a temporary sibling file and only replaces the target after the archive is complete, preventing a truncated final ZIP on failure. After success, the app asks Explorer to reveal the ZIP. Temporary session data is deleted when the session is replaced, saved, canceled, or the app exits.

## Tests (`root/tests`)

All automated tests live in `root/tests`; implementation directories contain no tests.

Test coverage is organized by behavior:

- **Profile-plan tests:** exact 69-PNG-plus-ICO count, filenames, relative paths, groups, and dimensions.
- **Image tests:** PNG/JPEG decoding, alpha conversion, EXIF orientation, square input, non-square containment, and explicit no-crop assertions.
- **Output tests:** decode every emitted PNG with WIC, validate dimensions, inspect ICO entries, and compare ZIP contents with the profile manifest.
- **Failure tests:** corrupt files, unsupported extensions/content, denied staging/write paths, and canceled/failed export all produce expected diagnostics without a partial user ZIP.
- **End-to-end fixtures:** deterministic source fixtures generate the complete reference matrix and a valid archive.
- **WinUI smoke tests:** file selection/drag-drop state, preview readiness, validation presentation, Save As cancellation, and successful Explorer-reveal handoff.

Tests should use a lightweight native C++ test executable and CTest integration rather than introducing a managed test runtime. Test source images and golden metadata live under `root/tests` with the test code.

## Non-goals

- command-line interface in the MVP;
- additional platform/store profiles or a user-visible profile selector;
- vector source support, image editing, crop controls, padding art direction, or automatic contrast redesign;
- cloud storage, uploads, accounts, analytics, telemetry, or background synchronization;
- editing an unrelated repository or carrying product-specific HeadsUp names into the new package.

