## Task 8: Complete the approved Asset Board presentation

After each step you will commit, push, get the ci, stop for independent review.

```markdown
**Files:**
- Modify: `root/windows/MainWindow.xaml`
- Modify: `root/windows/MainWindow.xaml.h`
- Modify: `root/windows/MainWindow.xaml.cpp`
- Modify: `root/windows/AssetBoardViewModel.h`
- Modify: `root/windows/AssetBoardViewModel.cpp`
- Modify: `root/windows/Project.idl`
- Modify only if required for presentation support: `root/windows/App.xaml`
- Modify only if required for source metadata: `root/windows/AssetBoardState.h`, `root/windows/AssetBoardState.cpp`
- Modify tests only for new state/view-model behavior that can be verified without a XAML host
- Remove or reduce temporary Task 7 diagnostic instrumentation only after the polished runtime path is proven stable

**Visual reference:**
The approved presentation direction is Option A, **Asset board**, from the design exploration.
`"C:\Users\User\Documents\Dev\C++\.superpowers\WAC\brainstorm\session-1789353190796\content\layout.html"`

Option A establishes the target hierarchy:

- compact persistent drop/browse strip
- source thumbnail and metadata card
- grouped generated-asset thumbnail sections
- visible validation-success summary
- Reset and Save As footer actions
- polished light Windows-style presentation

The reference shows the source card immediately below the compact intake strip, grouped thumbnail grids for AppList and logo families, a green validation summary, and Reset / Save As actions at the bottom. It is a presentation target, not a request to replace the working application architecture.

**Interfaces:**
- Consumes the proven Task 7 `AssetBoardState`, `AssetBoardViewModel`, grouped projected assets, staging-session preview paths, brokered image intake, Save As flow, and Explorer reveal.
- Produces the completed WinUI presentation for the MVP.
- Does not change the 69-PNG-plus-ICO output contract, generation pipeline, AppContainer broker boundaries, session ownership, ZIP behavior, or Explorer reveal architecture.

### Task 8 constraints

Task 8 is a presentation-completion task.

Preserve these proven Task 7 contracts unless a reproducible defect directly requires otherwise:

- `AssetBoardState` generation/save/reset semantics
- `GenerationSession` staging ownership
- `winrt::make` runtime-class projection
- owning `com_ptr` access to the view-model implementation
- the existing projected group/item view-model types
- `IObservableVector<AssetBoardItemViewModel>` for group asset collections
- current UI-dispatch completion boundaries
- brokered `StorageFile` intake
- app-owned temporary generation/export paths
- brokered `CopyAndReplaceAsync` Save As
- `LaunchFolderPathAsync` Explorer reveal
- the fixed 69 PNG + `AppIcon.ico` Store/MSIX profile

Do not use Task 8 as an opportunity for unrelated architecture cleanup.

Do not introduce:
- editor functionality
- crop controls
- profile selection
- image manipulation controls
- cloud features
- telemetry
- additional output formats
- alternate application shells
- broad filesystem capabilities
- `runFullTrust`

### Step 1: Populate real source presentation data

Replace the current placeholder source properties with metadata from the accepted source.

The ready presentation must expose at minimum:

- source filename
- source pixel dimensions
- source preview image
- the existing no-crop framing note

The source summary should resemble the approved Option A source card:

- small square thumbnail
- filename as primary text
- dimensions as secondary text
- framing note below

Do not expose private temporary paths as source metadata.

If source metadata must be carried through the generation boundary, add the smallest state/view-model fields required. Do not redesign `GenerationSession` unless the existing UI/state layer cannot safely retain this presentation data.

Fix the current dimension encoding defect so dimensions render as:

`1024 × 768`

rather than mojibake such as:

`1024 Ã— 768`

Verify the actual runtime result rather than assuming the source literal alone fixes encoding.

### Step 2: Keep source intake available after generation

The approved Asset Board keeps a compact source-intake control available after an image has loaded so the user can replace the source without restarting the app.

Adapt the existing Browse/drop presentation so:

- idle state presents the obvious empty drop target
- ready state presents a compact `Drop another image or browse…` strip
- dropping or browsing a new valid image begins a new generation through the existing Task 7 intake path
- the old ready session is replaced through existing state/session ownership
- Reset still returns the application to the clean idle state

Do not create a second intake implementation.

Both idle and replacement intake must converge on the existing `GenerateFromStorageFile` path.

### Step 3: Replace raw asset rows with actual thumbnail tiles

The generated board already contains the correct groups and projected assets.

Keep those collections.

Change only their presentation.

Each generated PNG item should show:

- actual staged image thumbnail
- asset filename or concise label
- dimensions where useful

Do not display the raw staged filesystem path in the normal user-facing UI.

Use the existing staged preview path as the source for the thumbnail presentation.

The approved direction uses compact square image tiles on a transparency-friendly surface. The thumbnail board should make the generated assets visually inspectable without turning the application into an image editor.

The ICO may use an appropriate representative preview or compact summary rather than requiring every internal ICO size to become a separate board item.

### Step 4: Present the existing groups as a polished asset board

Retain the existing underlying 8-group collection contract.

Visually organize it into clear family sections based on the approved Option A hierarchy.

The UI may visually consolidate related groups, for example:

- AppList
  - default
  - unplated
  - light-unplated

- Logo families
  - Square44
  - Square150
  - StoreLogo
  - MedTile

- AppIcon

This is presentation grouping only.

Do not change `StoreMsixProfile` or generation grouping merely to achieve the visual layout.

Use a scrollable thumbnail-grid presentation with sensible wrapping/responsiveness for the application window.

Avoid returning to nested-control experiments that alter the proven collection contract unless runtime evidence demonstrates a requirement.

### Step 5: Complete the validation and command presentation

Replace the current plain validation text with a polished success presentation matching the approved direction.

For a valid ready session show clearly:

- `69 PNG + 1 ICO ready`
- validation passed

Use a compact success treatment rather than a developer-style status line.

Preserve error presentation through the existing diagnostic state.

Complete the bottom command area:

- Reset as the secondary action
- Save As as the primary action

Keep the existing enablement rules:

- Save As enabled only when `CanSave`
- Reset enabled only when `CanReset`
- neither action should become available during inappropriate busy states

Do not change Save As behavior in this task.

### Step 6: Apply the approved light Windows visual system

Bring the working shell in line with Option A.

Target:

- light neutral application canvas
- white content surfaces/cards
- subtle Windows-style borders
- restrained blue accent for primary interaction
- soft green validation-success treatment
- compact spacing
- modest corner radii
- clear typography hierarchy
- no unnecessary decorative complexity

The application should feel like a small native Windows utility, not a web page reproduced literally in XAML.

Use WinUI resources and controls where practical rather than hardcoding every visual value.

The design reference is directional. Preserve native Windows behavior and accessibility over pixel-for-pixel HTML imitation.

### Step 7: Preserve and verify runtime image materialization

Thumbnail rendering introduces a new presentation path and must receive its own packaged-app smoke verification.

Verify:

1. PNG Browse
   - source thumbnail renders
   - all generated sections render
   - generated PNG thumbnails are visible
   - application remains alive

2. JPEG Browse
   - source thumbnail renders
   - generated thumbnails render
   - application remains alive

3. Drag/drop PNG
   - intake succeeds
   - thumbnails render

4. Source replacement
   - source A reaches ready
   - source B is chosen/dropped without restarting
   - source card updates to B
   - board updates to B
   - old staging session is removed

5. Reset
   - clears source presentation
   - clears thumbnails
   - returns to idle drop target

6. Unsupported/corrupt input
   - error presentation remains correct
   - Save As disabled
   - Reset remains available

7. Save As cancellation
   - ready presentation remains intact

8. Successful Save As
   - ZIP remains exactly 69 PNG + 1 ICO
   - Explorer reveal still succeeds
   - polished UI remains usable after save

### Step 8: Isolate Task 7 diagnostic instrumentation

Only after the polished packaged runtime flow has passed:

Preserve the Task 7 diagnostic tooling where it remains useful for future investigation, but ensure investigation-only diagnostics are inactive during normal application use.

- wall off projection/materialization trace noise behind an explicit diagnostic/development switch
- wall off property-getter tracing behind the same diagnostic boundary
- disable temporary XAML binding diagnostics during normal execution
- disable ZIP investigation-only diagnostics during normal execution
- keep the development flight recorder available for future debugging, but do not initialize or write to it unless diagnostics are explicitly enabled
- retain normal product diagnostics and actionable error reporting that have deliberate ongoing value
- prefer one clear diagnostic activation mechanism rather than scattered individual switches
- keep diagnostic code close to the behavior it observes so it can be reactivated without reconstructing the Task 7 investigation tooling

Default behavior must be:

- diagnostics disabled
- no development flight-recorder prompt or setup
- no continuous trace file creation
- no high-volume property/projection/materialization logging
- no synchronous per-event flush behavior during ordinary use
- no user-visible diagnostic UI
- Release builds must not continuously perform Task 7 investigation logging

Diagnostic mode may remain available for deliberate development/debug sessions.

Do not delete useful diagnostic infrastructure solely because Task 7 is complete.

Do not perform unrelated refactors while isolating diagnostics.

After changing the diagnostic activation boundary:

- rebuild Debug/package
- run native tests
- run the packaged smoke matrix relevant to affected paths
- verify normal execution does not create or continuously write investigation trace output
- where practical, briefly activate diagnostic mode and verify the preserved tooling still functions

The goal of this step is dormant, reusable diagnostics, not diagnostic removal.

### Step 9: Add or update focused automated tests

Add automated coverage only where the new behavior can be tested without manufacturing a XAML integration harness.

At minimum cover any newly introduced state required for:

- source name lifecycle
- source dimensions lifecycle
- Reset clearing source presentation
- second-source replacement updating source presentation

Do not introduce a managed test framework or large UI automation dependency solely for Task 8.

Existing generation/profile/output/session tests must remain unchanged and passing unless the presentation-state contract legitimately requires additions.

### Step 10: Build and verify the complete application

Run:

```powershell
msbuild root/WindowsAssetCreator.sln /t:Build /p:Configuration=Debug /p:Platform=x64
```

Run:

```powershell
root/x64/Debug/AssetCoreTests.exe
```

Expected:

- complete Debug|x64 solution builds
- all existing tests pass
- all newly added tests pass

Then package/install and execute the Task 8 runtime smoke matrix.

Also perform a Release|x64 build before closing Task 8 so Debug-only diagnostic behavior does not hide a Release configuration failure.

### Task 8 acceptance criteria

Task 8 is complete when:

- the application visually follows approved Option A
- Browse and drag/drop remain functional
- replacement intake works without restarting
- source filename and dimensions are accurate
- the source preview renders
- dimension text renders `×` correctly
- generated PNGs appear as actual thumbnails
- raw staging paths are no longer normal user-facing content
- grouped asset sections remain complete
- exactly 8 underlying groups / 70 generated assets remain represented
- validation success is clearly presented
- Reset works from ready and error states
- Save As cancellation preserves the ready presentation
- successful Save As still creates exactly 69 PNG + 1 ICO
- Explorer reveal still works
- previous staging sessions are cleaned up
- temporary Task 7 tracing has been removed or reduced to intentional diagnostics
- Debug|x64 build passes
- Release|x64 build passes
- native test suite passes
- complete packaged runtime smoke passes

### Task 8 gate: report and stop

Report only:

- files changed
- Debug build result
- Release build result
- native test result
- packaged runtime smoke result
- any genuine remaining presentation or functional concern

Do not create a task report file.

Do not automatically commit.

Do not begin release hardening, distribution, Store submission, packaging redesign, or additional features.

**Stop and wait for external review after Task 8.**
```
