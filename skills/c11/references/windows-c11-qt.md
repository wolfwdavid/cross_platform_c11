# Windows (c11-qt) — supported CLI subset

**Read this if you are running on the Windows build of c11 (`c11-qt`).** The rest of
this skill documents the **upstream/macOS** c11 CLI. The Windows port reimplements only
a subset, so most commands the skill teaches **do not exist on Windows yet** and will
error. This note is the honest contract for the Windows build: use what's here, don't
reach for the rest. (Full divergence detail: `docs/windows-cli-vs-skill-audit.md`.)

Status: the full Tier-1 agent-orchestration set has landed — `send`, `send-key`,
`read-screen`, `identify`, plus shell-integration env at spawn. An agent can now drive,
read, and self-locate in a c11-qt pane. Remaining gaps are Tier 2/3 (see below).

## Are you on c11-qt?

- **Shell-integration env vars are present** (as on macOS): `C11_SHELL_INTEGRATION=1`,
  `C11_SURFACE_ID`, `C11_WORKSPACE_ID`, and `C11_SOCKET` are exported into each pane's
  shell. Detection works like the main skill: check `C11_SHELL_INTEGRATION`. (Note:
  `C11_TAB_ID` is not set — c11-qt has one surface per pane, no in-pane tabs.) Run
  `c11 identify` to get your `surface_ref` / `workspace_ref`.
- **JSON-RPC output, not `OK` lines.** Every command returns a JSON-RPC envelope
  (`{"id":1,"ok":true,"result":…}`), never the macOS skill's `OK surface:N pane:P
  workspace:M` form. Any script that parses the documented `OK …`/`*_ref` shape breaks.
- **Socket is a named pipe.** The control socket is `\\.\pipe\c11-<USERNAME>` (override
  with `C11_SOCKET` or `CMUX_SOCKET`), not a `.sock` file under Application Support.

## What works today

| Area | Commands (CLI) | Notes |
|---|---|---|
| System | `ping`, `tree`, `capabilities` | |
| Workspaces | `list-workspaces`, `current-workspace`, `new-workspace`, `close-workspace`, `select-workspace`, `next-workspace`, `prev-workspace` | `new-workspace --title`; `select-workspace <n>` or `<id>` (positional) |
| Panes | `new-pane`, `new-split`, `close-surface`, `list-surfaces`, `list-panes` | `new-pane` is **terminal-only** (`--cwd`); `new-split <dir>` / `--direction` (+`--cwd`) |
| Browser | `open-browser` (positional URL or `--url`) | Qt WebEngine; renders real pages |
| Drive | `send` | `send [--surface <uuid>] "text"` types into a terminal surface **and** presses Return; `--no-submit` types without Return; `--text "…"` explicit. No `--surface` → the focused pane. Target by **UUID** (from `list-surfaces`/`tree`), not `surface:N`. |
| Drive | `send-key` | `send-key [--surface <uuid>] <chord>` sends a key chord: `enter`, `ctrl+c`, `shift+tab`, `alt+f4`, arrows (`up`/`down`/`left`/`right`), `escape`, `tab`, `f1`–`f12`, letters/digits. Case-insensitive; `+`-separated; modifiers `ctrl`/`alt`/`shift`/`super`. |
| Read | `read-screen` | `read-screen [--surface <uuid>] [--lines N] [--scrollback]` returns a terminal's rendered text (visible viewport by default; `--lines N` keeps the last N; `--scrollback` includes history). Prints the text directly. |
| Identity | `identify` | `identify` reports the calling surface's `surface_ref`/`pane_ref`/`workspace_ref` (+titles), read from `C11_SURFACE_ID` in the pane's env; `--surface <uuid>` overrides. |
| Status | `set-status`, `clear-status` | sidebar status text |

**Reachable only via raw `--json` method** (no friendly CLI alias on Windows):

- `surface.get_metadata` / `set_metadata` / `clear_metadata` — surface metadata
- `theme.get` / `theme.list` / `theme.set_active` — chrome theme
- `set_progress` / `clear_progress` — sidebar progress
- `navigate` — browser navigation (v1)

Invoke a raw method:

```bash
c11 --json --method surface.set_metadata --params '{"surface":"surface:1","title":"build"}'
```

## What's missing (do not use on Windows)

Documented elsewhere in this skill, **not implemented** in c11-qt:

- **Tier 1 — remaining:** `new-pane --type browser|markdown` (c11-qt `new-pane` is
  terminal-only) and `new-surface` (in-pane tabs). The core drive/read/identity set —
  `send`, `send-key`, `read-screen`, `identify` — now works (see above).
- **Tier 2 — metadata sugar:** `set-title`, `set-description`, `set-agent`,
  `default-agent`, `rename-tab`. Use the raw `surface.set_metadata` method instead.
- **Tier 3 — comms / sidebar / layout:** `mailbox`, `conversation`, `resize-pane`,
  `trigger-flash` / `cancel-flash`, `get-titlebar-state`, `list-status`, `list-snapshots`,
  workspace `--layout` blueprints, `--no-focus`, `--pane` targeting, and the `markdown`
  CLI (markdown is UI-only on Windows: File → Open Markdown / Ctrl+Shift+M).

Model note for Windows: one surface per pane (no surfaces-as-tabs), so anything in the
main skill that distinguishes panes from in-pane tabs doesn't apply yet.
