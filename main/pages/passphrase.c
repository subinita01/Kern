#include "passphrase.h"
#include "../ui/dialog.h"
#include "../ui/input_helpers.h"
#include "../ui/theme.h"
#include "../ui/theme_widgets.h"
#include "../utils/secure_mem.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

static lv_obj_t *passphrase_screen = NULL;
static ui_text_input_t text_input = {0};
static void (*return_callback)(void) = NULL;
static passphrase_success_callback_t success_callback = NULL;

// Two-step confirmation state
static bool confirming = false;
static char first_entry[512];
static lv_obj_t *title_label = NULL;
static lv_obj_t *strength_label = NULL;

// ---------------------------------------------------------------------------
// Strength indicator (mirrors kef_encrypt_page.c)
// ---------------------------------------------------------------------------

typedef enum {
  KEY_STRENGTH_NONE,
  KEY_STRENGTH_WEAK,
  KEY_STRENGTH_FAIR,
  KEY_STRENGTH_GOOD,
  KEY_STRENGTH_STRONG,
} key_strength_t;

static key_strength_t calculate_key_strength(const char *text) {
  if (!text || text[0] == '\0')
    return KEY_STRENGTH_NONE;

  size_t len = strlen(text);
  int has_lower = 0, has_upper = 0, has_digit = 0, has_symbol = 0;

  for (size_t i = 0; i < len; i++) {
    char c = text[i];
    if (c >= 'a' && c <= 'z')
      has_lower = 1;
    else if (c >= 'A' && c <= 'Z')
      has_upper = 1;
    else if (c >= '0' && c <= '9')
      has_digit = 1;
    else
      has_symbol = 1;
  }

  int classes = has_lower + has_upper + has_digit + has_symbol;

  if (len < 6)
    return KEY_STRENGTH_WEAK;
  if (len < 8)
    return (classes >= 3) ? KEY_STRENGTH_FAIR : KEY_STRENGTH_WEAK;
  if (len < 12)
    return (classes >= 3) ? KEY_STRENGTH_GOOD : KEY_STRENGTH_FAIR;
  return (classes >= 3) ? KEY_STRENGTH_STRONG : KEY_STRENGTH_GOOD;
}

static const char *strength_text(key_strength_t s) {
  switch (s) {
  case KEY_STRENGTH_WEAK:
    return "Weak";
  case KEY_STRENGTH_FAIR:
    return "Fair";
  case KEY_STRENGTH_GOOD:
    return "Good";
  case KEY_STRENGTH_STRONG:
    return "Strong";
  default:
    return "";
  }
}

static lv_color_t strength_color(key_strength_t s) {
  switch (s) {
  case KEY_STRENGTH_WEAK:
    return error_color();
  case KEY_STRENGTH_FAIR:
    return highlight_color();
  case KEY_STRENGTH_GOOD:
  case KEY_STRENGTH_STRONG:
    return good_color();
  default:
    return lv_color_white();
  }
}

static void passphrase_changed_cb(lv_event_t *e) {
  (void)e;
  if (!strength_label || !text_input.textarea)
    return;
  if (confirming) {
    lv_label_set_text(strength_label, "");
    return;
  }
  const char *text = lv_textarea_get_text(text_input.textarea);
  key_strength_t s = calculate_key_strength(text);
  if (s == KEY_STRENGTH_NONE) {
    lv_label_set_text(strength_label, "");
  } else {
    lv_label_set_text(strength_label, strength_text(s));
    lv_obj_set_style_text_color(strength_label, strength_color(s), 0);
  }
}

// ---------------------------------------------------------------------------
// Navigation callbacks
// ---------------------------------------------------------------------------

static void back_confirm_cb(bool result, void *user_data) {
  (void)user_data;
  if (result) {
    confirming = false;
    secure_memzero(first_entry, sizeof(first_entry));
    if (return_callback)
      return_callback();
  }
}

static void back_btn_cb(lv_event_t *e) {
  (void)e;
  if (confirming) {
    // On second step: go back to first entry without leaving the page
    confirming = false;
    secure_memzero(first_entry, sizeof(first_entry));
    lv_label_set_text(title_label, "Enter Passphrase");
    lv_textarea_set_text(text_input.textarea, "");
    return;
  }
  dialog_show_confirm("Are you sure you want to go back?", back_confirm_cb,
                      NULL, DIALOG_STYLE_OVERLAY);
}

// ---------------------------------------------------------------------------
// Two-step keyboard callback
// ---------------------------------------------------------------------------

static void keyboard_ready_cb(lv_event_t *e) {
  (void)e;
  const char *text = lv_textarea_get_text(text_input.textarea);

  if (!confirming) {
    // Phase 1: store entry, advance to confirmation step
    strncpy(first_entry, text, sizeof(first_entry) - 1);
    first_entry[sizeof(first_entry) - 1] = '\0';
    confirming = true;

    lv_label_set_text(title_label, "Confirm Passphrase");
    lv_textarea_set_text(text_input.textarea, "");
    if (strength_label)
      lv_label_set_text(strength_label, "");
    return;
  }

  // Phase 2: compare against first entry
  bool match = (strcmp(first_entry, text) == 0);
  secure_memzero(first_entry, sizeof(first_entry));
  confirming = false;

  if (!match) {
    lv_label_set_text(title_label, "Enter Passphrase");
    lv_textarea_set_text(text_input.textarea, "");
    dialog_show_error_timeout("Passphrases do not match.\nPlease try again.",
                              NULL, 0);
    return;
  }

  if (success_callback)
    success_callback(text);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void passphrase_page_create(lv_obj_t *parent, void (*return_cb)(void),
                            passphrase_success_callback_t success_cb) {
  (void)parent;
  return_callback = return_cb;
  success_callback = success_cb;
  confirming = false;
  secure_memzero(first_entry, sizeof(first_entry));

  passphrase_screen = theme_create_page_container(lv_screen_active());

  title_label = theme_create_page_title(passphrase_screen, "Enter Passphrase");

  ui_create_back_button(passphrase_screen, back_btn_cb);

  ui_text_input_create(&text_input, passphrase_screen, "passphrase", false,
                       keyboard_ready_cb);

  // Strength indicator — shown on first entry, hidden during confirmation
  strength_label = lv_label_create(passphrase_screen);
  lv_label_set_text(strength_label, "");
  lv_obj_set_style_text_font(strength_label, theme_font_small(), 0);
  lv_obj_set_width(strength_label, LV_PCT(100));
  lv_obj_set_style_text_align(strength_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(strength_label, text_input.textarea, LV_ALIGN_OUT_BOTTOM_MID,
                  0, 5);
  lv_obj_add_event_cb(text_input.keyboard, passphrase_changed_cb,
                      LV_EVENT_VALUE_CHANGED, NULL);
}

void passphrase_page_show(void) {
  if (passphrase_screen)
    lv_obj_clear_flag(passphrase_screen, LV_OBJ_FLAG_HIDDEN);
  if (text_input.keyboard)
    lv_obj_clear_flag(text_input.keyboard, LV_OBJ_FLAG_HIDDEN);
}

void passphrase_page_hide(void) {
  if (passphrase_screen)
    lv_obj_add_flag(passphrase_screen, LV_OBJ_FLAG_HIDDEN);
  if (text_input.keyboard)
    lv_obj_add_flag(text_input.keyboard, LV_OBJ_FLAG_HIDDEN);
}

void passphrase_page_destroy(void) {
  confirming = false;
  secure_memzero(first_entry, sizeof(first_entry));
  strength_label = NULL;
  title_label = NULL;
  ui_text_input_destroy(&text_input);
  if (passphrase_screen) {
    lv_obj_del(passphrase_screen);
    passphrase_screen = NULL;
  }
  return_callback = NULL;
  success_callback = NULL;
}
