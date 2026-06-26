# c11 Windows UI Target Layout

This document describes the target interface layout for the Windows (Qt/C++) version of c11, based on the macOS reference implementation.

## Reference Screenshot

See `Screenshot 2026-06-26 at 2.59.15 PM.png` in the repo root.

## Window Structure

The c11 window has three main regions:

### 1. Sidebar (left edge, ~170px wide)
- Dark background, slightly darker than terminal panes
- **Workspace list** at the top: shows workspace names (e.g. "Workspace 1"), each clickable to switch
- **"+" button** below the workspace list to create new workspaces
- **Bottom area** contains:
  - "Next Notification" label with arrow button
  - Up/down arrow buttons (workspace navigation)
  - Update availability indicator (e.g. "Update Available: 0...")
  - "THIS IS A DEV BUILD" label in red at the very bottom

### 2. Content area (right of sidebar, fills remaining space)
- Contains a **2x2 grid of panes** created by one vertical split and one horizontal split
- Each pane is a self-contained terminal container with its own tab bar

### 3. Pane Layout (2x2 grid)

Each pane has:

#### Tab bar (top of each pane)
- **Left side**: Close button (X), tab title (e.g. "✳ Claude Code" or "~")
- **Right side**: Toolbar icons in this order:
  - A (text/font)
  - Terminal icon
  - Globe icon (browser)
  - Document icon
  - Split vertical icon
  - Split horizontal icon
  - "+" (new tab)
  - X (close pane)
- Orange/amber accent color for active tab title and toolbar highlights
- Gray/muted for inactive panes

#### Tab content area
- Below the tab bar, a secondary row shows the active tab's title with a ">" chevron on the right
- The terminal content fills the remaining space

### Grid layout from the screenshot:

```
┌─────────────────────────┬─────────────────────────┐
│ Top-Left                │ Top-Right               │
│ "✳ Claude Code" tab     │ "~" tab (shell)         │
│ (Claude Code running)   │ (zsh prompt)            │
│ 50% width × 50% height  │ 50% width × 50% height  │
├─────────────────────────┼─────────────────────────┤
│ Bottom-Left             │ Bottom-Right            │
│ "~" tab (shell)         │ "~" tab (shell)         │
│ (zsh prompt)            │ (zsh prompt)            │
│ 50% width × 50% height  │ 50% width × 50% height  │
└─────────────────────────┴─────────────────────────┘
```

## Color Theme (Dark)

- **Window background / sidebar**: Very dark gray (~#1a1a1a)
- **Terminal background**: Dark gray (~#2a2a2a)
- **Tab bar background**: Slightly lighter dark gray (~#333333)
- **Active tab/accent**: Orange/amber (~#e88a1a)
- **Inactive tab text**: Gray (#888888)
- **Terminal text**: Light gray/white (#cccccc)
- **Divider lines between panes**: Subtle dark line (~#444444)
- **"THIS IS A DEV BUILD"**: Red text at sidebar bottom

## Key Behaviors

1. **Pane splitting**: Any pane can be split vertically or horizontally using the toolbar icons (split-V and split-H buttons in each pane's tab bar)
2. **Tab management**: Each pane supports multiple tabs. The "+" button adds a new tab to that pane. Tabs can be terminal shells, Claude Code sessions, browser views, or markdown surfaces.
3. **Workspace switching**: Clicking a workspace in the sidebar switches the entire content area to that workspace's pane layout
4. **Focus indication**: The active/focused pane has orange accent colors in its tab bar; inactive panes have gray/muted tab bars
5. **Title bar**: Shows "Workspace 1" (or current workspace name) centered, with standard window controls (close/minimize/maximize) on the left

## GhosttyKit Build Fix

The `build-ghosttykit` CI was failing because `src/renderer/Metal.zig` had an exhaustive switch on the `PlatformTag` enum that didn't handle `.qt`. The fix: add `.qt => unreachable` since Qt uses OpenGL, not Metal. This fix has been pushed to `wolfwdavid/ghostty` on both `main` and `windows-build-fixes` branches.

To pick up the fix in your branch:
```bash
cd ghostty
git fetch origin
git pull origin windows-build-fixes
cd ..
git add ghostty
git commit -m "Update ghostty submodule with Metal qt fix"
```
