---
name: c11-qt-run
description: "Launch, drive, and screenshot the Windows c11-qt app (Qt/C++ port) to see a change working. Use when asked to run, start, screenshot, or manually verify the Windows build — not the macOS app (see c11-hotload) and not the test suite."
---

# c11-qt run

How to launch the **Windows** `c11-qt` app, drive it over the socket, and screenshot
the window to confirm a change works. This is the maintainer "see it run" path for the
Qt/C++ port; the macOS app uses `c11-hotload` instead, and tests use the c11-qt test
policy (`build_msvc.bat test`).

## Prereqs

- A built app. From `c11-qt/`: `build_msvc.bat` (or `build_msvc.bat test` to also run
  ctest). Output binaries:
  - **App:** `c11-qt/build/bin/c11.exe` (links `ghostty.dll` next to it).
  - **CLI:** `c11-qt/build/cli/c11.exe` — *also* named `c11.exe`. Don't confuse them;
    the CLI is your driver, the bin one is the GUI.
- Qt runtime DLLs on PATH. Default here is `C:\Qt\6.11.1\msvc2022_64\bin` (override via
  `C11_QT_BIN`). The build scripts pin this Qt version.

## Launch — and the PATH gotcha

Launch with `scripts/launch.bat` (sets PATH, cd's to `build/bin`, `start`s the app):

```bash
/c/Windows/System32/cmd.exe //c "skills\c11-qt-run\scripts\launch.bat"   # from Git Bash
```

**Critical:** launch with a real *Windows* PATH (`C:\Windows\System32;…;<Qt>\bin`).
The app's terminal panes spawn the default shell — **PowerShell** on Windows
(`pwsh.exe` if present, else `powershell.exe`; via `platform::defaultShellCommand()`),
not the bare `cmd.exe` ghostty would otherwise use — and that shell **inherits the
launcher's environment**. If you launch from Git Bash with its Unix-style `$PATH`
(`/c/Windows/System32`), the pane shell can't be resolved/started — the app looks fine,
but nothing runs in any terminal. `launch.bat` sets the correct PATH so spawned shells
work. (The app binary itself launches fine from anywhere; it's the *child shells* that
need the Windows PATH.)

**PATH gotcha for `claude` (and other PATH-installed tools):** `launch.bat` sets a
*minimal* PATH that intentionally omits the user's PATH, so npm-global tools like
`claude` (`%APPDATA%\npm`) won't resolve in panes launched via the harness. To exercise
those tools, launch normally so panes inherit the full user PATH:
`( cd c11-qt/build/bin && /c/Windows/System32/cmd.exe //c start "" c11.exe )` with the
Qt bin dir prepended to `$PATH` first. `ls` works regardless (it's a PowerShell alias).

The PowerShell **tool** has been observed broken in some sessions (every call returns
exit 1, no output). Workarounds used throughout this recipe: drive `.bat` via
`/c/Windows/System32/cmd.exe //c "…"`, and run `powershell.exe` directly from Bash for
screenshots.

## The socket

The control socket is a **named pipe**: `\\.\pipe\c11-<USERNAME>` (override with
`C11_SOCKET` / `CMUX_SOCKET`). The CLI auto-resolves it. A pane's shell also has
`C11_SOCKET` injected, so a CLI run *inside* a pane auto-connects.

Wait for the app to bind the pipe before driving — `ping` until `pong`:

```bash
CLI=c11-qt/build/cli/c11.exe
for i in $(seq 1 12); do [ "$($CLI ping 2>&1 | grep -c pong)" -ge 1 ] && break; sleep 1; done
```

`ERROR: … QLocalSocket::connectToServer: Invalid name` just means the pipe isn't up yet
(app still starting or not running) — keep polling or relaunch.

## Drive it (don't just launch it)

```bash
CLI=c11-qt/build/cli/c11.exe
$CLI capabilities                                   # v1/v2/browser, version
$CLI new-workspace "Run Demo"                        # make a clean space
NID=$($CLI new-pane | grep -oE '[0-9a-f-]{36}')      # add a terminal pane (returns its UUID)
$CLI send --surface "$NID" 'echo hello && ver'       # type + submit a command
$CLI read-screen --surface "$NID" --lines 8          # read the rendered text back
$CLI identify --surface "$NID"                        # caller surface/workspace refs
```

Refs on c11-qt are **UUIDs** (from `list-surfaces` / `list-panes` / `tree`), not
`surface:N`. See `skills/c11/references/windows-c11-qt.md` for the full supported subset.

### Oracles that don't need pixels
- **`send` worked?** Send a command with a filesystem side effect and check the file:
  `$CLI send --surface "$NID" 'echo ok>C:\Users\<you>\probe.txt'` then test the file.
  This proves bytes reached the PTY *and* Enter submitted.
- **Pane actually placed in the layout?** `list-panes` reads the layout tree
  (`orderedPanelIds`), so its count reflects what will render — an orphaned pane wouldn't
  appear.

## Screenshot the window — and look at it

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "skills\c11-qt-run\scripts\capture-window.ps1" "C:\Users\<you>\AppData\Local\Temp\c11.png"
```

`capture-window.ps1` finds the app window by `c11.exe`'s PID, brings it to the
foreground, and saves a PNG. **Open the PNG and look** — a blank frame is a failed
launch, not a success.

**Occlusion caveat:** `CopyFromScreen` captures screen pixels, so if another window (e.g.
the editor) covers the c11 window you'll screenshot *that*. The script calls
`SetForegroundWindow`, but Windows can refuse foreground changes from a background
process — if your capture shows the wrong app, click the c11 window once and retry, or
minimize the occluder.

**GL-capture caveat (important):** GDI `CopyFromScreen` does **not** reliably capture the
**OpenGL**-rendered terminal surface (ghostty draws via WGL). A pane that is fully
visible to the user can come back **blank/black** in the screenshot — the screenshot is
lying, the pane isn't. **Do not conclude a rendering bug from a blank screenshot.** Trust
`read-screen`: if it returns the pane's content, the terminal is rendering. Use the
screenshot for chrome/layout (sidebar, splits, which workspace is selected), not as proof
of terminal-content rendering. (`PrintWindow` has the same GL limitation.)

## Known issues seen during runs
- **One-time unexpected exit:** the app has been observed to exit on its own shortly
  after the first drive sequence on a fresh launch, then run stably on relaunch. If it
  vanishes (`tasklist /FI "IMAGENAME eq c11.exe"` shows nothing), relaunch with logging:
  edit `launch.bat` to run the bin `c11.exe` in the foreground with `> log.txt 2>&1`
  (instead of `start`) to capture any crash output.
- **"Blank pane" in screenshots is usually a capture artifact, not a real bug.** Earlier
  notes here claimed socket-created panes render blank; that was wrong — confirmed by the
  operator that the terminal renders fine on screen. The blank came from GDI
  `CopyFromScreen` not capturing the OpenGL surface (see the GL-capture caveat above).
  Always cross-check a blank screenshot against `read-screen` before believing it.
  (A genuine reparent/GL repaint glitch may still exist in some cases, but don't diagnose
  it from screenshots alone.)

## Cleanup

```bash
/c/Windows/System32/taskkill.exe //IM c11.exe //F
```
