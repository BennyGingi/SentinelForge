# Color System

Parent brief: [`docs/BRIEF.md`](../BRIEF.md) — Part II §1–2.  
Token values: **generated from [`cmake/theme.cmake`](../../cmake/theme.cmake)** (single source of truth). Hex tables in this file document that set; do not hand-edit a parallel palette.

See also: [`desktop-design-spec.md`](desktop-design-spec.md).

---

## Rationale

SentinelForge desktop is a **12-hour-shift instrument panel**. The screen should stay calm so that real alerts read as shocks in peripheral vision.

### Cool slate surfaces

Surfaces use a cool slate family (`BgBase` → `BgRaised` / `BgInset`), never pure black `#000` or pure white `#FFF`. Cool neutrals:

- Keep overall luminance and saturation low at rest.
- Separate panels, headers, and inset mono areas without competing warm cast.
- Leave headroom so warm severity hues are the only bright, attention-grabbing colors on screen.

### Single accent

There is exactly one interactive accent (`Accent` / `AccentDim`). It means **interactive or active** only: active nav, focus ring, selection edge, primary button, progress fill.

It must never indicate a threat, operational status, or metric magnitude. If a color could be mistaken for an alert, it is the wrong color there. Confidence bars and similar non-threat metrics use the accent for “this is UI chrome,” not warmth-by-value.

### Severity owns warm hues

All warm hues belong to severity (and aligned status error/warn where intentional): Critical / High / Medium warm foregrounds and badge backgrounds; Low uses a cool blue; Info stays muted gray.

Severity is **never color alone**: every level carries a text label and a positional cue (the 3px left severity rail on detection rows). That supports the grayscale / accessibility acceptance checks in Issue #018.

Operational status dots (`StatusOk`, `StatusWarn`, `StatusError`, `StatusIdle`) are 8px indicators — not emoji — and reuse the same restrained vocabulary.

---

## Token tables (generated from `cmake/theme.cmake`)

> **Generated from `cmake/theme.cmake`.** Edit the CMake file; regenerate `Theme.h` / `dark.qss`. Do not copy these hex values into C++ or a second stylesheet by hand.

### Surfaces

| Token | Hex | Use |
|---|---|---|
| `BgBase` | `#0B0E14` | Application background, nav rail |
| `BgSurface` | `#121620` | Panels, cards, table body |
| `BgRaised` | `#171C28` | Table headers, hovered rows, chips |
| `BgInset` | `#0E1219` | Log viewer, mono areas, inspector |
| `BorderSubtle` | `#1F2633` | Panel edges, separators |
| `BorderStrong` | `#2A3342` | Focused inputs, splitter handles |

### Text

| Token | Hex | Use |
|---|---|---|
| `TextPrimary` | `#E4E9F2` | Values, cells, log body |
| `TextSecondary` | `#9AA6B8` | Labels, headers, inactive nav |
| `TextMuted` | `#6B7688` | Units, resting timestamps, empty states |

### Accent

| Token | Hex | Use |
|---|---|---|
| `Accent` | `#38D6C4` | Active nav, focus ring, selection edge, primary button |
| `AccentDim` | `#1E6E68` | Progress fill, subdued accent backgrounds |

### Severity

| Level | Foreground | Badge background | Rail |
|---|---|---|---|
| Critical | `#FF4D6A` | `#2A0F17` | `#FF4D6A` |
| High | `#FF8A3D` | `#2A1710` | `#FF8A3D` |
| Medium | `#FFC53D` | `#2A2210` | `#FFC53D` |
| Low | `#4DA3FF` | `#10202E` | `#4DA3FF` |
| Info | `#7C8899` | `#161B24` | `#7C8899` |

### Operational status

| Token | Hex |
|---|---|
| `StatusOk` | `#3ECF8E` |
| `StatusWarn` | `#FFC53D` |
| `StatusError` | `#FF4D6A` |
| `StatusIdle` | `#6B7688` |

---

## Hard exclusions

- No second accent.
- No gradients, glows, or drop shadows as color substitutes.
- No default Qt blue selection — selection uses `BgRaised` + accent edge cues from the design spec.
