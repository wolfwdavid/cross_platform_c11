# Windows (c11-qt) — supported CLI subset

**Read this if you are running on the Windows build of c11 (`c11-qt`).** The rest of
this skill documents the **upstream/macOS** c11 CLI. The Windows port reimplements only
a subset, so most commands the skill teaches **do not exist on Windows yet** and will
error. This note is the honest contract for the Windows build: use what's here, don't
reach for the rest. (Full divergence detail: `docs/windows-cli-vs-skill-audit.md`.)

Status: stopgap. `send` and `send-key` have landed; the remaining high-value primitives
(`read-screen`, `identify`, …) are a planned Tier-1 milestone — until they land, treat
them as absent.

## Are you on c11-qt?

Two reliable tells — the macOS detection in the main skill **does not work here**:

- **No shell-integration env vars.** c11-qt does *not* export `C11_SHELL_INTEGRATION`,
  `C11_SURFACE_ID`, `C11_WORKSPACE_ID`, or `C11_TAB_ID` into your shell. If you're in a
  c11 terminal but those are all empty, you're almost certainly on c11-qt. (Consequence:
  you cannot self-identify or self-target from env vars — see "What's missing" below.)
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

- **Tier 1 — orchestration core (remaining):** `read-screen`, `identify`,
  `new-pane --type browser|markdown`, `new-surface` (tabs within a pane). `send` and
  `send-key` now work (see above). Without `identify` and shell-integration env vars you
  still cannot reliably self-target; without `read-screen` you cannot read a peer
  agent's pane back.
- **Tier 2 — metadata sugar:** `set-title`, `set-description`, `set-agent`,
  `default-agent`, `rename-tab`. Use the raw `surface.set_metadata` method instead.
- **Tier 3 — comms / sidebar / layout:** `mailbox`, `conversation`, `resize-pane`,
  `trigger-flash` / `cancel-flash`, `get-titlebar-state`, `list-status`, `list-snapshots`,
  workspace `--layout` blueprints, `--no-focus`, `--pane` targeting, and the `markdown`
  CLI (markdown is UI-only on Windows: File → Open Markdown / Ctrl+Shift+M).

Model note for Windows: one surface per pane (no surfaces-as-tabs), so anything in the
main skill that distinguishes panes from in-pane tabs doesn't apply yet.
