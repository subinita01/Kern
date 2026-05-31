# Kern UI Design Guidelines  
  
This document is the authoritative reference for UI design decisions in Kern.  
All color, icon, font, and layout choices must follow these guidelines to ensure  
consistency across the codebase and all supported hardware targets.  
  
---  
  
## Table of Contents  
  
1. [Color System](#1-color-system)  
2. [Icons & Symbols](#2-icons--symbols)  
3. [Typography](#3-typography)  
4. [Dialog Types & Semantic Usage](#4-dialog-types--semantic-usage)  
5. [Layout & Sizing](#5-layout--sizing)  
  
---  
  
## 1. Color System  
  
All colors are defined as `#define` macros in `main/ui/theme.c` and exposed  
exclusively through accessor functions declared in `main/ui/theme.h`.  
  
**Never use raw hex values in UI code. Always call the theme function.**  
  
### 1.1 Color Reference  
  
| Semantic Intent      | Function              | Hex       | Usage                                              |  
|----------------------|-----------------------|-----------|----------------------------------------------------|  
| Background           | `bg_color()`          | `0x000000`| Screen and overlay backgrounds                     |  
| Panel                | `panel_color()`       | `0x1a1a1a`| Dialog and card backgrounds                        |  
| Primary text         | `main_color()`        | `0xFFFFFF`| Button labels, primary content                     |  
| Secondary text       | `secondary_color()`   | `0x888888`| Labels created via `theme_create_label()`          |  
| Accent / Title       | `highlight_color()`   | `0xff6600`| Dialog titles, page titles, dropdown borders       |  
| Disabled             | `disabled_color()`    | `0x333333`| Unavailable buttons, btnmatrix item backgrounds    |  
| Error / Negative     | `error_color()`       | `0xFF0000`| Error messages, danger dialog borders, "No" in normal confirm |  
| Negative action      | `no_color()`          | `0xFF0000`| "No" button label in normal confirm dialogs        |  
| Positive action      | `yes_color()`         | `0x00FF00`| "Yes" button label in normal confirm dialogs       |  
| Cyan accent          | `cyan_color()`        | `0x00FFFF`| Available for future use; not currently used in dialogs |  
  
> `error_color()` and `no_color()` both return `0xFF0000`. They are separate  
> functions to express different semantic intent in code.  
  
### 1.2 Semantic Color Mapping by Dialog State  
  
| Dialog State | Border          | Message text      | "Yes" label   | "No" label    |  
|--------------|-----------------|-------------------|---------------|---------------|  
| Info         | `main_color()`  | `secondary_color()` | `main_color()` | —            |  
| Confirm      | `main_color()`  | `secondary_color()` | `yes_color()` | `no_color()`  |  
| Danger       | `error_color()` | `secondary_color()` | `no_color()`  | `yes_color()` |  
| Error        | `main_color()`  | `error_color()`   | —             | —             |  
| Progress     | `main_color()`  | `secondary_color()` | —            | —             |  
  
> **Danger button reversal:** In danger dialogs, "Yes" is colored `no_color()`  
> (red) and "No" is colored `yes_color()` (green). This is intentional — the  
> safe choice ("No") is visually prominent. See `show_confirm_internal()` in  
> `main/ui/dialog.c`.  
  
### 1.3 Rules  
  
1. **Never hardcode hex values** — always call the theme accessor function.  
2. **One semantic color per intent** — do not use `error_color()` for  
   non-error states.  
3. **Dark background assumption** — all colors are designed for a black  
   (`0x000000`) background. Do not use them on light backgrounds.  
4. `secondary_color()` (`0x888888`) has a contrast ratio of ~1.9:1 against  
   black. It is acceptable for non-critical secondary text only. Never use it  
   for actionable or safety-critical messages.  
  
---  
  
## 2. Icons & Symbols  
  
### 2.1 Custom Icon Font  
  
Kern uses a custom icon font generated from **FontAwesome 7 Free Solid** via  
`lv_font_conv`. Three sizes are compiled into the firmware:  
  
| File                          | Size  | Used as fallback for  |  
|-------------------------------|-------|-----------------------|  
| `main/ui/assets/icons_16.c`   | 16 px | `theme_font_small()` on narrow screens |  
| `main/ui/assets/icons_24.c`   | 24 px | `theme_font_small()` on wide screens; `theme_font_medium()` on narrow screens |  
| `main/ui/assets/icons_36.c`   | 36 px | `theme_font_medium()` on wide screens |  
  
The icon fonts are set as **fallback fonts** on the Montserrat font structs  
during `theme_init()`. Any label using `theme_font_small()` or  
`theme_font_medium()` can render these icons by embedding the codepoint in the  
label text.  
  
### 2.2 Available Icon Codepoints  
  
The following codepoints are compiled into the icon fonts  
(see generation command in `main/ui/assets/icons_16.h`):  
  
| Codepoint | FontAwesome name | Suggested use          |  
|-----------|------------------|------------------------|  
| `0xE0B4`  | *(FA7 glyph)*    | TBD — verify visually  |  
| `0xF029`  | `fa-qrcode`      | QR code actions        |  
| `0xF126`  | `fa-code-fork`   | Version / branch info  |  
| `0xF577`  | `fa-sd-card`     | SD card / storage      |  
  
> **Note:** The current icon set contains **functional** icons only. There are  
> no semantic icons (warning triangle, checkmark, X/close) in the compiled  
> font. To add semantic icons for dialogs, regenerate the font with additional  
> codepoints (e.g., `0xF071` for `fa-triangle-exclamation`, `0xF00C` for  
> `fa-check`, `0xF00D` for `fa-xmark`) and document them here.  
  
### 2.3 Rendering an Icon in a Label  
  
```c  
// Embed the codepoint directly in a UTF-8 string.  
// The label must use theme_font_small() or theme_font_medium().  
lv_obj_t *icon = lv_label_create(parent);  
lv_obj_set_style_text_font(icon, theme_font_medium(), 0);  
lv_obj_set_style_text_color(icon, error_color(), 0);  
lv_label_set_text(icon, "\xEE\x82\xB4");  // UTF-8 for U+E0B4  
```  
  
### 2.4 Icon Placement Rules  
  
1. Icons appear **above** the message text when used in dialogs.  
2. Icons are **centered horizontally** (`LV_TEXT_ALIGN_CENTER`).  
3. Icons use the same font as dialog body text (`theme_font_medium()`).  
4. Icon color must match the semantic intent of the dialog (see §1.2).  
5. Icons **reinforce** text — never use an icon as the sole indicator of  
   meaning.  
  
### 2.5 Semantic Icon Assignments (Target State)  
  
Once the icon font is extended with semantic glyphs, use this mapping:  
  
| Dialog state | Icon glyph         | Color           |  
|--------------|--------------------|-----------------|  
| Confirm      | `fa-check` (✓)     | `yes_color()`   |  
| Danger       | `fa-triangle-exclamation` (⚠) | `error_color()` |  
| Error        | `fa-xmark` (✕)     | `error_color()` |  
| Info         | `fa-circle-info` (ℹ) | `main_color()` |  
  

---  
  
## 3. Typography  
  
### 3.1 Font Sizes  
  
Font sizes are responsive to screen width, resolved once in `theme_init()`:  
  
| Screen width | `theme_font_small()` | `theme_font_medium()` |  
|---|---|---|  
| < 600 px     | Montserrat 16 px     | Montserrat 24 px      |  
| ≥ 600 px     | Montserrat 24 px     | Montserrat 36 px      |  
  
Both fonts have the custom icon font set as a fallback.  
  
### 3.2 Font Usage by Element  
  
| Element              | Font                  | Color                  |  
|----------------------|-----------------------|------------------------|  
| Dialog title         | `theme_font_medium()` | `highlight_color()`    |  
| Dialog message       | `theme_font_medium()` | `secondary_color()`    |  
| Error message        | `theme_font_medium()` | `error_color()`        |  
| Button label         | `theme_font_medium()` | Semantic (see §1.2)    |  
| Page title           | `theme_font_small()`  | `secondary_color()`    |  
| Secondary/hint text  | `theme_font_small()`  | `secondary_color()`    |  
| Btnmatrix items      | `theme_font_small()`  | `main_color()`         |  
  
### 3.3 Rules  
  
1. Use only `theme_font_small()` and `theme_font_medium()` — never reference  
   `lv_font_montserrat_*` directly in UI code.  
2. Dialog body text always uses `theme_font_medium()` for readability across  
   all supported screen sizes.  
  
---  
  
## 4. Dialog Types & Semantic Usage  
  
Six dialog types are available in `main/ui/dialog.h`. Choose based on the  
decision tree below.  
  
```  
Does the user need to take an action?  
├── Yes — is it destructive/irreversible?  
│   ├── Yes → dialog_show_danger_confirm()   [red border, swapped buttons]  
│   └── No  → dialog_show_confirm()          [standard Yes/No]  
└── No  
    ├── Is an error being reported?          → dialog_show_error()     [auto-close]  
    ├── Is an operation in progress?         → dialog_show_progress()  [no buttons]  
    ├── Is there a title + message to show?  → dialog_show_info()      [OK button]  
    └── Legacy fixed-size modal              → dialog_show_message()   [avoid in new code]  
```  
  
### 4.1 `dialog_show_info()`  
  
- **Purpose:** Neutral information; user dismisses with OK.  
- **Title:** `highlight_color()` (orange), `theme_font_medium()`  
- **Message:** `secondary_color()` (gray), `theme_font_medium()`  
- **OK button:** `main_color()` (white) label  
- **Style:** `DIALOG_STYLE_OVERLAY` or `DIALOG_STYLE_FULLSCREEN`  
  
### 4.2 `dialog_show_confirm()`  
  
- **Purpose:** User chooses to proceed with a safe, reversible action.  
- **Message:** `secondary_color()` (gray)  
- **"Yes" label:** `yes_color()` (green)  
- **"No" label:** `no_color()` (red)  
- **Border:** `main_color()` (white, from `theme_apply_frame()`)  
  
### 4.3 `dialog_show_danger_confirm()`  
  
- **Purpose:** Confirm a **destructive or irreversible** action.  
- **Message:** `secondary_color()` (gray)  
- **"Yes" label:** `no_color()` (red) — visually warns the user  
- **"No" label:** `yes_color()` (green) — visually promotes the safe choice  
- **Border (overlay only):** `error_color()` (red), 2 px  
  
### 4.4 `dialog_show_error()`  
  
- **Purpose:** Report a failure; auto-dismisses after `timeout_ms`.  
- **Title:** "Error" in `secondary_color()` (gray)  
- **Message:** `error_color()` (red)  
- **Hint:** "Returning..." in `secondary_color()`  
- **Default timeout:** 2000 ms if `timeout_ms ≤ 0`  
- **No buttons** — do not use for actions requiring user confirmation.  
  
### 4.5 `dialog_show_progress()`  
  
- **Purpose:** Indicate an ongoing operation with no user interaction.  
- **Returns** the root `lv_obj_t *` — caller must `lv_obj_del()` it when done.  
- **Title:** `highlight_color()` (orange)  
- **Message:** `secondary_color()` (gray)  
- **No buttons, no auto-close.**  
  
### 4.6 `dialog_show_message()` *(legacy)*  
  
- Fixed 400×220 px — not responsive. Avoid in new code; use `dialog_show_info()`  
  instead.  
  
---  
  
## 5. Layout & Sizing  
  
All sizing constants are proportional to screen width, computed once in  
`theme_init()` and accessed via theme functions:  
  
| Constant                        | Formula (@ 720 px wide) | Typical value |  
|---------------------------------|-------------------------|---------------|  
| `theme_get_button_width()`      | `scr_w * 5 / 24`        | 150 px        |  
| `theme_get_button_height()`     | `scr_w * 5 / 36`        | 100 px        |  
| `theme_get_button_spacing()`    | `scr_w / 36`            | 20 px         |  
| `theme_get_default_padding()`   | `scr_w / 24`            | 30 px         |  
| `theme_get_min_touch_size()`    | `scr_w / 8`             | 90 px         |  
| `theme_get_small_padding()`     | `scr_w / 72`            | 10 px         |  
  
### Overlay dialog constraints  
  
- Width: `LV_PCT(90)` of screen width  
- Height: auto-fitted to content, capped at **80% of screen height**  
- Dialog frame: `theme_apply_frame()` — `COLOR_PANEL` background, white 2 px  
  border, 6 px radius  
- Overlay blocker: full-screen, 50% opaque black  
  
### Rules  
  
1. Never hardcode pixel values for dialog or button sizes — use theme functions.  
2. Message labels inside dialogs use `LV_PCT(90)` width with  
   `LV_LABEL_LONG_WRAP`.  
3. Confirm dialog buttons: `LV_PCT(40)` width each, aligned bottom-left and  
   bottom-right.  
4. All interactive elements must be at least `theme_get_min_touch_size()` (90 px  
   at 720 px wide) in their smallest dimension.
