<!-- AICB:BEGIN {"version":1,"target":".github/copilot-instructions.md","generatedAt":"2026-09-03T00:48:47.893Z","hash":"sha256:8029d7333f854a194af1af64d93c0cd7ce3324d5bed1d37550e0d7c1d5641a51"} -->
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
- `.cursorrules` _(spec)_ — Cursor rules
- `.windsurfrules` _(spec)_ — Windsurf rules
- `.github/copilot-instructions.md` _(spec)_ — GitHub Copilot instructions (.github)
- `AGENT.md` _(spec)_ — Multi-agent instructions
- `samples/irixsrc/irix-657m-src/README.md` _(spec)_ — Project README (samples/irixsrc/irix-657m-src)
- `samples/netbsd/usr (2)/usr/src/usr.bin/xlint/lint1/README.md` _(spec)_ — Project README (samples/netbsd/usr (2)/usr/src/usr.bin/xlint/lint1)
- `samples/netbsd/usr (2)/usr/src/external/public-domain/sqlite/sqlite2mdoc/README.md` _(spec)_ — Project README (samples/netbsd/usr (2)/usr/src/external/public-domain/sqlite/sqlite2mdoc)
- `samples/netbsd/usr (2)/usr/src/usr.bin/indent/README.md` _(spec)_ — Project README (samples/netbsd/usr (2)/usr/src/usr.bin/indent)
- `samples/netbsd/usr (2)/usr/src/README.md` _(spec)_ — Project README (samples/netbsd/usr (2)/usr/src)
- `samples/linux/tools/sched_ext/README.md` _(spec)_ — Project README (samples/linux/tools/sched_ext)
- `samples/linux/rust/zerocopy/README.md` _(spec)_ — Project README (samples/linux/rust/zerocopy)
- `samples/linux/rust/zerocopy-derive/README.md` _(spec)_ — Project README (samples/linux/rust/zerocopy-derive)
- `samples/linux/rust/syn/README.md` _(spec)_ — Project README (samples/linux/rust/syn)
- `samples/linux/rust/quote/README.md` _(spec)_ — Project README (samples/linux/rust/quote)
- `samples/linux/rust/proc-macro2/README.md` _(spec)_ — Project README (samples/linux/rust/proc-macro2)
- `samples/linux/rust/pin-init/README.md` _(spec)_ — Project README (samples/linux/rust/pin-init)
- `samples/irixsrc/IRIX-6.5.17-Src/README.md` _(spec)_ — Project README (samples/irixsrc/IRIX-6.5.17-Src)
- `samples/linux/drivers/gpu/drm/amd/display/dc/dml2_0/README.md` _(spec)_ — Project README (samples/linux/drivers/gpu/drm/amd/display/dc/dml2_0)

## Working files (current focus)
- `emu/src/cpu_interface.cpp` _(auto:recent-edit)_
- `emu/tests/CMakeLists.txt` _(auto:recent-edit)_
- `emu/tests/test_cpu_interface.cpp` _(auto:recent-edit)_
- `emu/include/o2emu/bus.h` _(auto:recent-edit)_
- `emu/include/o2emu/logging.h` _(auto:recent-edit)_
- `emu/src/memory_map.cpp` _(auto:recent-edit)_
- `emu/src/main.cpp` _(auto:recent-edit)_
- `emu/src/vice.cpp` _(auto:recent-edit)_
- `emu/src/crime.cpp` _(auto:recent-edit)_
- `emu/src/mace.cpp` _(auto:recent-edit)_
- `emu/include/o2emu/cpu_interface.h` _(auto:recent-edit)_
- `emu/include/o2emu/memory_map.h` _(auto:recent-edit)_
- `.aicb/state.json` _(auto:recent-edit)_
- `.vscode/settings.json` _(auto:recent-edit)_

## How to use this handoff
1. Read every file under "Spec / context files" before acting.
2. Continue the work described in the most recent thought.
3. Honor skill statuses: `ENABLED` use freely, `ASK` require explicit user confirmation each time, `DISABLED` must not be used.
4. When you reach a non-trivial decision, append a thought to `.aicb/state.json` (modelId + text + sourceReference if relevant).
<!-- AICB:END -->
