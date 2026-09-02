<!-- AICB:BEGIN {"version":1,"target":".kilocode/rules/aicb.md","generatedAt":"2026-09-02T08:56:59.676Z","hash":"sha256:8898c2cdb51b1020a1275d98292271ab615d1049f1c658b1443d7570182a9379"} -->
# AI Context Bridge — Handoff

Workspace: `/home/samuel/SGI Projects/O2Emu`

## Spec / context files (read these first)
- `docs/README.md` _(spec)_ — Project README (docs)
- `README.md` _(spec)_ — Project README
- `AGENTS.md` _(spec)_ — Multi-agent instructions
- `ROADMAP.md` _(spec)_ — Spec: ROADMAP.md
- `CLAUDE.md` _(spec)_ — Claude Code instructions
- `.agent/AGENTS.md` _(spec)_ — Multi-agent instructions (.agent)
- `GEMINI.md` _(spec)_ — Gemini instructions
- `AGENT.md` _(spec)_ — Multi-agent instructions
- `.cursorrules` _(spec)_ — Cursor rules
- `.windsurfrules` _(spec)_ — Windsurf rules
- `.github/copilot-instructions.md` _(spec)_ — GitHub Copilot instructions (.github)

## How to use this handoff
1. Read every file under "Spec / context files" before acting.
2. Continue the work described in the most recent thought.
3. Honor skill statuses: `ENABLED` use freely, `ASK` require explicit user confirmation each time, `DISABLED` must not be used.
4. When you reach a non-trivial decision, append a thought to `.aicb/state.json` (modelId + text + sourceReference if relevant).
<!-- AICB:END -->
