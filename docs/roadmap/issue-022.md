# Issue #022 — Identity and final polish

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part II §1, §7–8.  
Architecture: [`docs/architecture/gui-architecture.md`](../architecture/gui-architecture.md).

**Prerequisite:** Issue #021 (operator controls).

Final identity, interaction polish, and persistence. No new pages.

---

## Scope

### 022.1 — Visual identity

Do not generate a shield. Pick one direction:

- **Crucible aperture** *(recommended)* — hexagonal opening, off-center spark; forge + sensor aperture; earns the "Forge" name.
- **Struck spark** — single geometric spark, no container; works at 16px.
- **Anvil monogram** — two-stroke anvil resolving into `S`.

Constraints: two strokes max, legible at 16px, monochrome-first (accent on one element only), no gradient/bevel/glow.

### 022.2 — Icon and branding plumbing

- `gui/resources/branding/logo.svg` — mark + horizontal lockup.
- Multi-resolution `.ico` at 16, 24, 32, 48, 64, 128, 256 px (16px needs simplified drawing).
- Windows `.rc` with `IDI_ICON1` via `target_sources`; `QApplication::setWindowIcon`.
- About dialog: mark, version, build hash, Qt version, license, project URL — Settings + Help → About.

### 022.3 — Interaction polish

Functional motion only:

- **New critical row** — single 400 ms background fade (severity badge → normal); one flash, no repeat.
- **Hover/selection** — instant; no transition.
- **Inspector open/close** — 180 ms splitter width via `QVariantAnimation`.
- **Nav page change** — instant.
- **Reduced motion** — platform setting + Settings toggle disable critical flash and inspector animation.

### 022.4 — Remaining polish

- Empty states: distinct copy for no-data, filtered-to-zero, disconnected.
- Alignment audit: panel title baselines, table column offsets, micro-label tracking.
- Focus rings on every interactive element — full dashboard tabbable without mouse.
- `QSettings` persistence: geometry, splitters, inspector state, nav rail expansion, active page.

## Deferred

Frameless custom title bar · cloud deployment branding.

---

## Acceptance checklist

- [ ] Icon legible at 16px in the Windows taskbar.
- [ ] Executable shows custom icon in Explorer.
- [ ] No shield in the mark.
- [ ] Reduced-motion setting disables critical-row flash and inspector animation.
- [ ] Grayscale screenshot: severity identifiable by rail position and pill label.
- [ ] Every setting in 022.4 survives a restart.
