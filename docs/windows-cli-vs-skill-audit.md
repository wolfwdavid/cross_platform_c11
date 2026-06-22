# c11-qt (Windows port) CLI vs. the c11 skill — divergence audit

**Date:** 2026-06-22
**Why:** `skills/c11/SKILL.md` (+ peers `c11-browser`, `c11-markdown`) documents the
**upstream/macOS** c11 CLI. The Windows `c11-qt` port reimplements only a subset in
`c11-qt/cli/c11cli.cpp` + `c11-qt/src/socket/SocketCommandRouter.cpp`. An agent that
reads the skill will try commands that don't exist on the Windows build.

This is a gap report — no code changes. It sizes the divergence so we can decide:
**(A)** implement the missing commands in c11-qt, or **(B)** ship a c11-qt-scoped
skill that only documents what actually works.

Method: enumerated the CLI's command map + the router's registered `m_v1Commands`
/ `m_v2Methods`, and grepped the skill files for `c11 <command>` usage, then
verified each candidate against the router source.

## Implemented in c11-qt (works today)

| Area | Commands (CLI) | Router method | Notes |
|---|---|---|---|
| System | `ping`, `tree`, `capabilities` | `system.*` | |
| Workspaces | `list-workspaces`, `current-workspace`, `new-workspace`, `close-workspace`, `select-workspace`, `next-workspace`, `prev-workspace` | `workspace.*` | `new-workspace --title` works; `select-workspace <n>`/`<id>` works (after the 2026-06-22 positional-arg fix) |
| Panes/surfaces | `new-pane`, `new-split`, `close-surface`, `list-surfaces`, `list-panes` | `surface.create`/`split`/`close`/`list`, `pane.list` | `new-pane` is **terminal-only** (+`--cwd`); `new-split <dir>`/`--direction` + `--cwd` |
| Browser | `open-browser` (`--url`/positional), `navigate` (v1) | `browser.open_split` | Qt WebEngine; renders real pages |
| Metadata | *(no CLI alias)* | `surface.get_metadata`/`set_metadata`/`clear_metadata` | reachable only via `--json` raw method |
| Status | `set-status`, `clear-status` | v1 `set_status`/`clear_status`, `set_progress`/`clear_progress` | |
| Theme | *(no CLI alias)* | `theme.get`/`list`/`set_active` | reachable only via `--json` |

## Missing vs. the skill (documented, NOT implemented)

Grouped by impact on the operator:agent mission.

**Tier 1 — core agent orchestration (highest value):**
- `send` — send text/commands to another surface. **The skill's central primitive** (~13 uses). No router handler.
- `send-key` — send key chords to a surface.
- `read-screen` — read a surface's rendered content back.
- `identify` — caller identity + `caller.pane_ref`/`surface_ref` (the skill uses these to self-target).
- `new-pane --type browser|markdown` / `--url` / `--file` — c11-qt `new-pane` only makes terminals.
- `new-surface` — add a surface as a **tab** within an existing pane (c11-qt has no tabs-within-pane CLI).

**Tier 2 — titles / metadata ergonomics (the skill leans on these constantly):**
- `set-title`, `set-description`, `set-agent`, `default-agent`, `rename-tab` — friendly metadata writes. c11-qt has only the raw `surface.set_metadata` method (no aliases, different shape).

**Tier 3 — comms + sidebar + layout:**
- `mailbox`, `conversation` — agent-to-agent message bus (~15 + 10 skill uses).
- `trigger-flash`, `cancel-flash`, `get-titlebar-state`, `list-status` — sidebar telemetry.
- `resize-pane` — rebalance splits (skill explicitly tells agents to use it after splits).
- `--layout` (workspace blueprints), `--no-focus`, `--pane <ref>` targeting flags.
- `list-snapshots`, snapshot management.
- `markdown` CLI open — c11-qt markdown is **UI-only** (File→Open Markdown / Ctrl+Shift+M); no socket/CLI path.

## Partial / divergent (present but different)

- **Output envelopes differ.** Skill expects v1-style `OK surface:<N> pane:<P> workspace:<M>` and `*_ref` handles; c11-qt returns JSON-RPC envelopes (`{"id":1,"ok":true,"result":…}`). Scripts/agents that parse the skill's documented output shape will break.
- **Pane/surface model.** Skill distinguishes panes vs. surfaces-as-tabs; c11-qt has panes only (one surface per pane).
- **No `cmux` provenance commands / shell-integration env** beyond the basics.

## Recommendation

The divergence is large but the **agent-facing value is concentrated in Tier 1**.
Two viable paths:

1. **Implement a Tier-1 milestone** (`send`, `send-key`, `read-screen`, `identify`,
   `new-pane --type`, `new-surface`). This is what turns the Windows port from
   "a terminal you can split" into "a room an agent can drive" — the c11 thesis.
   `send`/`read-screen` need a way to target a surface's ghostty handle and
   inject/read text (ghostty already has `ghostty_surface_text`; reading needs a
   screen-scrape seam). Estimate: each is a focused command; Tier 1 as a set is a
   real milestone, not a one-shot.

2. **Stopgap: a c11-qt-scoped skill note** that marks which commands work on the
   Windows build, so agents don't try missing ones. Cheap, honest, immediate —
   but caps what agents can do on Windows.

Suggested order: ship the stopgap note now (prevents agents failing on missing
commands), then implement Tier 1 as the next real milestone, deferring
Tier 2/3. Tier 2 (titles/metadata aliases) is a thin layer over the existing
`surface.*_metadata` methods and is a cheap follow-on once Tier 1 lands.
