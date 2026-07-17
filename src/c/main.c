// ---------------------------------------------------------------------------
// Clean Overview Watchface
// ---------------------------------------------------------------------------
// Digital clock with configurable seconds display, Bluetooth indicator,
// and health stats. Targets Pebble Time 2 (Emery, 200x228, colour).
// ---------------------------------------------------------------------------

#include <pebble.h>
#include "icons.h"

// --- layout constants --------------------------------------------------

#define BT_ICON_X        8
#define BT_ICON_Y        6
#define BT_ICON_W        14
#define BT_ICON_H        18

#define QT_ICON_W        14
#define QT_ICON_H        18
#define QT_ICON_X        (200 - QT_ICON_W - 8)
#define QT_ICON_Y        6

#define TIME_Y           45
#define TIME_H           44
#define DATE_H           34

#define STAT_TEXT_X_OFFSET  (ICON_BOX_W + 2)
#define STAT_TEXT_Y_OFFSET  -6
#define STAT_ROW_GAP_PX    4
#define STAT_TEXT_H        34

#define PEEK_HEIGHT_PX     51
#define STAT_PEEK_MARGIN_PX -10

// --- persistent storage keys -------------------------------------------

#define PERSIST_KEY_DARK_MODE           1
#define PERSIST_KEY_SECONDS_MODE        2
#define PERSIST_KEY_SHAKE_DURATION      3
#define PERSIST_KEY_VIBRATE_ON_DISCONNECT 4
#define PERSIST_KEY_SHOW_WEEK             5
#define PERSIST_KEY_SLOT_TOP_LEFT         6
#define PERSIST_KEY_SLOT_TOP_RIGHT        7
#define PERSIST_KEY_SLOT_BOTTOM_LEFT      8
#define PERSIST_KEY_SLOT_BOTTOM_RIGHT     9

// --- stat slot types -----------------------------------------------------

#define SLOT_NONE     0
#define SLOT_HEART    1
#define SLOT_STEPS    2
#define SLOT_CALORIES 3
#define SLOT_BATTERY  4

// --- seconds modes -----------------------------------------------------

#define SECONDS_OFF              0
#define SECONDS_ALWAYS           1
#define SECONDS_ON_SHAKE         2
#define SECONDS_ON_BACKLIGHT     3
#define SECONDS_ON_SHAKE_OR_BL   4

// --- palette -----------------------------------------------------------

typedef struct {
  GColor bg;
  GColor text;
  GColor heart;
  GColor steps;
  GColor flame;
  GColor batt_good;
  GColor batt_med;
  GColor batt_low;
  GColor bt_on;
  GColor bt_off;
} Palette;

static Palette s_palette;

static void apply_palette(bool dark_mode) {
  if (dark_mode) {
    s_palette.bg        = GColorBlack;
    s_palette.text      = GColorWhite;
    s_palette.heart     = GColorFromRGB(248,  60, 140);
    s_palette.steps     = GColorFromRGB( 20, 211, 245);
    s_palette.flame     = GColorFromRGB(255, 159,  51);
    s_palette.batt_good = GColorFromRGB(  0, 166,  41);
    s_palette.batt_med  = GColorFromRGB(255, 170,   0);
    s_palette.batt_low  = GColorFromRGB(255,   0,   0);
    s_palette.bt_on     = GColorFromRGB( 20, 211, 245);
    s_palette.bt_off    = GColorFromRGB(255,   0,   0);
  } else {
    s_palette.bg        = GColorWhite;
    s_palette.text      = GColorBlack;
    s_palette.heart     = GColorFromRGB(170,   0,  85);
    s_palette.steps     = GColorFromRGB(  0,  85, 170);
    s_palette.flame     = GColorFromRGB(255, 102,   0);
    s_palette.batt_good = GColorFromRGB(  0, 166,  41);
    s_palette.batt_med  = GColorFromRGB(255, 102,   0);
    s_palette.batt_low  = GColorFromRGB(255,   0,   0);
    s_palette.bt_on     = GColorFromRGB(  0,  85, 170);
    s_palette.bt_off    = GColorFromRGB(255,   0,   0);
  }
}

static GColor battery_colour(int percent) {
  if (percent > 40) return s_palette.batt_good;
  if (percent > 20) return s_palette.batt_med;
  return s_palette.batt_low;
}

// --- state -------------------------------------------------------------

static Window    *s_window;
static TextLayer *s_time_layer;
static TextLayer *s_sec_layer;
static TextLayer *s_ampm_layer;
static TextLayer *s_date_layer;
static Layer     *s_stats_layer;
static Layer     *s_bt_layer;
static Layer     *s_qt_layer;

static GFont s_time_font;
static GFont s_sub_font;
static GFont s_regular_font;

static char s_time_buf[12];
static char s_sec_buf[4];
static char s_ampm_buf[4];
static char s_date_buf[32];

static int  s_hr = 0;
static int  s_steps_val = 0;
static int  s_calories = 0;
static int  s_battery_percent = 0;
static bool s_charging = false;
static bool s_bt_connected = true;
static bool s_quiet_time = false;

static bool s_dark_mode = true;
static int  s_seconds_mode = SECONDS_OFF;
static int  s_shake_duration = 5;
static bool s_vibrate_on_disconnect = true;
static bool s_show_week = true;
static int  s_slot_top_left = SLOT_HEART;
static int  s_slot_top_right = SLOT_STEPS;
static int  s_slot_bottom_left = SLOT_CALORIES;
static int  s_slot_bottom_right = SLOT_BATTERY;

static AppTimer *s_seconds_timer = NULL;
static bool      s_showing_seconds = false;

// --- helpers -----------------------------------------------------------

static void format_stat(int value, char *out, size_t out_size) {
  if (value >= 10000) {
    snprintf(out, out_size, "%dK", value / 1000);
  } else {
    snprintf(out, out_size, "%d", value);
  }
}

// Forward declaration
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);

static void apply_tick_subscription(void) {
  bool need_seconds = (s_seconds_mode == SECONDS_ALWAYS) || s_showing_seconds;
  tick_timer_service_subscribe(need_seconds ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
}

// --- time/date update --------------------------------------------------

static void update_time_display(struct tm *tick_time) {
  bool show_seconds = (s_seconds_mode == SECONDS_ALWAYS) || s_showing_seconds;
  bool is_24h = clock_is_24h_style();

  if (!s_time_layer) return;

  // Adjust time layer width: full width in 24h, reduced in 12h to leave room for sub-layers
  Layer *time_l = text_layer_get_layer(s_time_layer);
  GRect tf = layer_get_frame(time_l);
  tf.size.w = is_24h ? 200 : 155;
  layer_set_frame(time_l, tf);

  if (is_24h) {
    strftime(s_time_buf, sizeof(s_time_buf),
             show_seconds ? "%H:%M:%S" : "%H:%M", tick_time);
    text_layer_set_text(s_time_layer, s_time_buf);
    if (s_sec_layer) {
      text_layer_set_text(s_sec_layer, "");
      layer_set_hidden(text_layer_get_layer(s_sec_layer), true);
    }
    if (s_ampm_layer) {
      text_layer_set_text(s_ampm_layer, "");
      layer_set_hidden(text_layer_get_layer(s_ampm_layer), true);
    }
  } else {
    strftime(s_time_buf, sizeof(s_time_buf), "%I:%M", tick_time);
    text_layer_set_text(s_time_layer, s_time_buf);

    if (s_sec_layer) {
      if (show_seconds) {
        // Reset to stacked position for 12h mode
        Layer *sl = text_layer_get_layer(s_sec_layer);
        GRect sf = layer_get_frame(sl);
        sf.origin.y = TIME_Y + 7;
        layer_set_frame(sl, sf);
        strftime(s_sec_buf, sizeof(s_sec_buf), ":%S", tick_time);
        text_layer_set_text(s_sec_layer, s_sec_buf);
        layer_set_hidden(sl, false);
      } else {
        text_layer_set_text(s_sec_layer, "");
        layer_set_hidden(text_layer_get_layer(s_sec_layer), true);
      }
    }

    if (s_ampm_layer) {
      strftime(s_ampm_buf, sizeof(s_ampm_buf), "%p", tick_time);
      text_layer_set_text(s_ampm_layer, s_ampm_buf);
      layer_set_hidden(text_layer_get_layer(s_ampm_layer), false);
    }
  }

  // Localized date format
  const char *locale = i18n_get_system_locale();
  char date_core[20];

  if (strncmp(locale, "de", 2) == 0) {
    static const char *de_days[] = {"So","Mo","Di","Mi","Do","Fr","Sa"};
    static const char *de_months[] = {"Jan","Feb","Mar","Apr","Mai","Jun",
                                       "Jul","Aug","Sep","Okt","Nov","Dez"};
    snprintf(date_core, sizeof(date_core), "%s, %d. %s",
             de_days[tick_time->tm_wday], tick_time->tm_mday,
             de_months[tick_time->tm_mon]);
  } else {
    strftime(date_core, sizeof(date_core), "%a, %b %e", tick_time);
  }

  if (s_show_week) {
    char week_buf[4];
    strftime(week_buf, sizeof(week_buf), "%V", tick_time);
    snprintf(s_date_buf, sizeof(s_date_buf), "%s | W%s", date_core, week_buf);
  } else {
    strncpy(s_date_buf, date_core, sizeof(s_date_buf));
  }
  text_layer_set_text(s_date_layer, s_date_buf);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time_display(tick_time);
  bool qt = quiet_time_is_active();
  if (qt != s_quiet_time) {
    s_quiet_time = qt;
    layer_mark_dirty(s_qt_layer);
  }
}

// --- seconds on shake --------------------------------------------------

static void seconds_timer_callback(void *data) {
  s_seconds_timer = NULL;
  s_showing_seconds = false;
  apply_tick_subscription();
  // Update display immediately to remove seconds
  time_t now = time(NULL);
  update_time_display(localtime(&now));
}

static void trigger_seconds_display(void) {
  s_showing_seconds = true;
  apply_tick_subscription();

  if (s_seconds_timer) {
    app_timer_cancel(s_seconds_timer);
  }
  s_seconds_timer = app_timer_register(s_shake_duration * 1000,
                                        seconds_timer_callback, NULL);

  time_t now = time(NULL);
  update_time_display(localtime(&now));
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_seconds_mode == SECONDS_ON_SHAKE || s_seconds_mode == SECONDS_ON_SHAKE_OR_BL) {
    trigger_seconds_display();
  }
  if (s_seconds_mode == SECONDS_ON_BACKLIGHT || s_seconds_mode == SECONDS_ON_SHAKE_OR_BL) {
    light_enable_interaction();
  }
}

// --- backlight handler -------------------------------------------------

static void backlight_handler(bool on) {
  if (s_seconds_mode != SECONDS_ON_BACKLIGHT && s_seconds_mode != SECONDS_ON_SHAKE_OR_BL) return;

  if (on) {
    // Backlight activated — cancel any fallback timer, let backlight control duration
    if (s_seconds_timer) {
      app_timer_cancel(s_seconds_timer);
      s_seconds_timer = NULL;
    }
    s_showing_seconds = true;
  } else {
    s_showing_seconds = false;
  }
  apply_tick_subscription();
  time_t now = time(NULL);
  update_time_display(localtime(&now));
}

// --- Bluetooth layer ---------------------------------------------------

static void bt_update_proc(Layer *layer, GContext *ctx) {
  GColor color = s_bt_connected ? s_palette.bt_on : s_palette.bt_off;

  // Draw Bluetooth rune symbol
  int x = BT_ICON_X;
  int y = BT_ICON_Y;
  int cx = x + BT_ICON_W / 2;

  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);

  // Vertical line
  graphics_draw_line(ctx, GPoint(cx, y), GPoint(cx, y + BT_ICON_H));
  // Top-right arrow
  graphics_draw_line(ctx, GPoint(cx, y), GPoint(cx + 5, y + 5));
  graphics_draw_line(ctx, GPoint(cx + 5, y + 5), GPoint(cx - 5, y + 9));
  // Bottom-right arrow
  graphics_draw_line(ctx, GPoint(cx, y + BT_ICON_H), GPoint(cx + 5, y + BT_ICON_H - 5));
  graphics_draw_line(ctx, GPoint(cx + 5, y + BT_ICON_H - 5), GPoint(cx - 5, y + BT_ICON_H - 9));

  // If disconnected, draw X over it
  if (!s_bt_connected) {
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_line(ctx, GPoint(x, y + 2), GPoint(x + BT_ICON_W, y + BT_ICON_H - 2));
    graphics_draw_line(ctx, GPoint(x, y + BT_ICON_H - 2), GPoint(x + BT_ICON_W, y + 2));
  }
}

static void bluetooth_callback(bool connected) {
  if (!connected && s_bt_connected && s_vibrate_on_disconnect) {
    vibes_double_pulse();
  }
  s_bt_connected = connected;
  layer_mark_dirty(s_bt_layer);
}

// --- quiet time layer --------------------------------------------------

static void qt_update_proc(Layer *layer, GContext *ctx) {
  if (!s_quiet_time) return;

  GColor color = s_palette.text;
  GRect bounds = layer_get_bounds(layer);

  // Draw a "moon" crescent centered in the layer
  int cx = bounds.size.w / 2;
  int cy = bounds.size.h / 2;
  int r = 9;
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_circle(ctx, GPoint(cx, cy), r);
  graphics_context_set_fill_color(ctx, s_palette.bg);
  graphics_fill_circle(ctx, GPoint(cx + 5, cy - 3), r - 1);
}

// --- stats layer -------------------------------------------------------

// Draws one stat slot (icon + text) at the given box position.
// Returns via out params nothing; draws directly.
static void draw_slot(GContext *ctx, int slot_type, int x, int y) {
  if (slot_type == SLOT_NONE) return;

  char buf[8];
  GColor color;

  switch (slot_type) {
    case SLOT_HEART:
      icons_draw_heart(ctx, x, y, s_palette.heart, s_palette.bg);
      format_stat(s_hr, buf, sizeof(buf));
      color = s_palette.heart;
      break;
    case SLOT_STEPS:
      icons_draw_steps(ctx, x, y, s_palette.steps);
      format_stat(s_steps_val, buf, sizeof(buf));
      color = s_palette.steps;
      break;
    case SLOT_CALORIES:
      icons_draw_flame(ctx, x, y, s_palette.flame, s_palette.bg);
      format_stat(s_calories, buf, sizeof(buf));
      color = s_palette.flame;
      break;
    case SLOT_BATTERY:
      color = battery_colour(s_battery_percent);
      icons_draw_battery(ctx, x, y, s_palette.text, s_palette.bg, color,
                         s_battery_percent, s_charging);
      format_stat(s_battery_percent, buf, sizeof(buf));
      break;
    default:
      return;
  }

  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, buf, s_regular_font,
    GRect(x + STAT_TEXT_X_OFFSET, y + STAT_TEXT_Y_OFFSET, 80, STAT_TEXT_H),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void stats_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, s_palette.bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int xLeft = bounds.size.w * 5 / 100;
  int xRight = bounds.size.w * 55 / 100;
  int yRow1 = 0;
  int yRow2 = ICON_BOX_H + STAT_ROW_GAP_PX;

  draw_slot(ctx, s_slot_top_left, xLeft, yRow1);
  draw_slot(ctx, s_slot_top_right, xRight, yRow1);
  draw_slot(ctx, s_slot_bottom_left, xLeft, yRow2);
  draw_slot(ctx, s_slot_bottom_right, xRight, yRow2);
}

// --- battery handler ---------------------------------------------------

static void battery_handler(BatteryChargeState state) {
  s_battery_percent = state.charge_percent;
  s_charging = state.is_charging;
  layer_mark_dirty(s_stats_layer);
}

// --- health handler ----------------------------------------------------

#if defined(PBL_HEALTH)
static void refresh_health(void) {
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  HealthServiceAccessibilityMask mask;
  mask = health_service_metric_accessible(HealthMetricStepCount, start, end);
  if (mask & HealthServiceAccessibilityMaskAvailable) {
    s_steps_val = (int)health_service_sum_today(HealthMetricStepCount);
  }

  mask = health_service_metric_accessible(HealthMetricActiveKCalories, start, end);
  if (mask & HealthServiceAccessibilityMaskAvailable) {
    s_calories = (int)health_service_sum_today(HealthMetricActiveKCalories)
               + (int)health_service_sum_today(HealthMetricRestingKCalories);
  }

  HealthValue bpm = health_service_peek_current_value(HealthMetricHeartRateBPM);
  if (bpm > 0) s_hr = (int)bpm;

  layer_mark_dirty(s_stats_layer);
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventSignificantUpdate ||
      event == HealthEventMovementUpdate ||
      event == HealthEventHeartRateUpdate) {
    refresh_health();
  }
}
#endif

// --- mode/repaint ------------------------------------------------------

static void repaint_for_mode(void) {
  window_set_background_color(s_window, s_palette.bg);
  text_layer_set_text_color(s_time_layer, s_palette.text);
  text_layer_set_text_color(s_sec_layer, s_palette.text);
  text_layer_set_text_color(s_ampm_layer, s_palette.text);
  text_layer_set_text_color(s_date_layer, s_palette.text);
  layer_mark_dirty(s_stats_layer);
  layer_mark_dirty(s_bt_layer);
  layer_mark_dirty(s_qt_layer);
  layer_mark_dirty(window_get_root_layer(s_window));
}

// --- AppMessage inbox --------------------------------------------------

static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  Tuple *t;

  t = dict_find(iter, MESSAGE_KEY_DARK_MODE);
  if (t) {
    s_dark_mode = (t->value->int32 != 0);
    persist_write_bool(PERSIST_KEY_DARK_MODE, s_dark_mode);
    apply_palette(s_dark_mode);
    repaint_for_mode();
  }

  t = dict_find(iter, MESSAGE_KEY_SECONDS_MODE);
  if (t) {
    s_seconds_mode = atoi(t->value->cstring);
    persist_write_int(PERSIST_KEY_SECONDS_MODE, s_seconds_mode);
    s_showing_seconds = false;
    if (s_seconds_timer) {
      app_timer_cancel(s_seconds_timer);
      s_seconds_timer = NULL;
    }
    apply_tick_subscription();
    time_t now = time(NULL);
    update_time_display(localtime(&now));
  }

  t = dict_find(iter, MESSAGE_KEY_SHAKE_DURATION);
  if (t) {
    s_shake_duration = atoi(t->value->cstring);
    persist_write_int(PERSIST_KEY_SHAKE_DURATION, s_shake_duration);
  }

  t = dict_find(iter, MESSAGE_KEY_SHOW_WEEK);
  if (t) {
    s_show_week = (t->value->int32 != 0);
    persist_write_bool(PERSIST_KEY_SHOW_WEEK, s_show_week);
    time_t now = time(NULL);
    update_time_display(localtime(&now));
  }

  t = dict_find(iter, MESSAGE_KEY_VIBRATE_ON_DISCONNECT);
  if (t) {
    s_vibrate_on_disconnect = (t->value->int32 != 0);
    persist_write_bool(PERSIST_KEY_VIBRATE_ON_DISCONNECT, s_vibrate_on_disconnect);
  }

  t = dict_find(iter, MESSAGE_KEY_SLOT_TOP_LEFT);
  if (t) {
    s_slot_top_left = atoi(t->value->cstring);
    persist_write_int(PERSIST_KEY_SLOT_TOP_LEFT, s_slot_top_left);
    layer_mark_dirty(s_stats_layer);
  }

  t = dict_find(iter, MESSAGE_KEY_SLOT_TOP_RIGHT);
  if (t) {
    s_slot_top_right = atoi(t->value->cstring);
    persist_write_int(PERSIST_KEY_SLOT_TOP_RIGHT, s_slot_top_right);
    layer_mark_dirty(s_stats_layer);
  }

  t = dict_find(iter, MESSAGE_KEY_SLOT_BOTTOM_LEFT);
  if (t) {
    s_slot_bottom_left = atoi(t->value->cstring);
    persist_write_int(PERSIST_KEY_SLOT_BOTTOM_LEFT, s_slot_bottom_left);
    layer_mark_dirty(s_stats_layer);
  }

  t = dict_find(iter, MESSAGE_KEY_SLOT_BOTTOM_RIGHT);
  if (t) {
    s_slot_bottom_right = atoi(t->value->cstring);
    persist_write_int(PERSIST_KEY_SLOT_BOTTOM_RIGHT, s_slot_bottom_right);
    layer_mark_dirty(s_stats_layer);
  }
}

// --- window load/unload ------------------------------------------------

static void main_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  window_set_background_color(window, s_palette.bg);

  s_time_font = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  s_sub_font = fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
  s_regular_font = fonts_get_system_font(FONT_KEY_GOTHIC_28);

  // Time layer - HH:MM (or HH:MM:SS in 24h mode)
  // Left-aligned with offset to visually center the group
  int time_w = bounds.size.w;
  s_time_layer = text_layer_create(GRect(0, TIME_Y, time_w, 56));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, s_palette.text);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_time_layer));

  // Seconds sub-layer (top-right of time, for 12h mode)
  int sub_x = 155;
  s_sec_layer = text_layer_create(GRect(sub_x, TIME_Y + 7, 42, 22));
  text_layer_set_background_color(s_sec_layer, GColorClear);
  text_layer_set_text_color(s_sec_layer, s_palette.text);
  text_layer_set_font(s_sec_layer, s_sub_font);
  text_layer_set_text_alignment(s_sec_layer, GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(s_sec_layer));

  // AM/PM sub-layer (below seconds, for 12h mode)
  s_ampm_layer = text_layer_create(GRect(sub_x, TIME_Y + 28, 42, 22));
  text_layer_set_background_color(s_ampm_layer, GColorClear);
  text_layer_set_text_color(s_ampm_layer, s_palette.text);
  text_layer_set_font(s_ampm_layer, s_sub_font);
  text_layer_set_text_alignment(s_ampm_layer, GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(s_ampm_layer));

  // Stats layer
  int stats_h = ICON_BOX_H + STAT_ROW_GAP_PX + STAT_TEXT_H;
  int stats_y = bounds.size.h - PEEK_HEIGHT_PX - stats_h - STAT_PEEK_MARGIN_PX;
  s_stats_layer = layer_create(GRect(0, stats_y, bounds.size.w, stats_h));
  layer_set_update_proc(s_stats_layer, stats_update_proc);
  layer_add_child(root, s_stats_layer);

  // Date layer - in the peek zone at bottom
  int date_y = bounds.size.h - PEEK_HEIGHT_PX + (PEEK_HEIGHT_PX - DATE_H) / 2;
  s_date_layer = text_layer_create(GRect(0, date_y, bounds.size.w, DATE_H));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, s_palette.text);
  text_layer_set_font(s_date_layer, s_regular_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_date_layer));

  // Bluetooth layer - top-left
  s_bt_layer = layer_create(GRect(0, 0, BT_ICON_X + BT_ICON_W + 4, BT_ICON_Y + BT_ICON_H + 4));
  layer_set_update_proc(s_bt_layer, bt_update_proc);
  layer_add_child(root, s_bt_layer);

  // Quiet time layer - top-right
  s_qt_layer = layer_create(GRect(200 - 28, 0, 28, 28));
  layer_set_update_proc(s_qt_layer, qt_update_proc);
  layer_add_child(root, s_qt_layer);
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_sec_layer);
  text_layer_destroy(s_ampm_layer);
  text_layer_destroy(s_date_layer);
  layer_destroy(s_stats_layer);
  layer_destroy(s_bt_layer);
  layer_destroy(s_qt_layer);
}

// --- init / deinit / main ----------------------------------------------

static void init(void) {
  // Load persisted settings
  s_dark_mode = persist_exists(PERSIST_KEY_DARK_MODE)
                ? persist_read_bool(PERSIST_KEY_DARK_MODE) : true;
  s_seconds_mode = persist_exists(PERSIST_KEY_SECONDS_MODE)
                   ? persist_read_int(PERSIST_KEY_SECONDS_MODE) : SECONDS_OFF;
  s_shake_duration = persist_exists(PERSIST_KEY_SHAKE_DURATION)
                     ? persist_read_int(PERSIST_KEY_SHAKE_DURATION) : 5;
  s_vibrate_on_disconnect = persist_exists(PERSIST_KEY_VIBRATE_ON_DISCONNECT)
                            ? persist_read_bool(PERSIST_KEY_VIBRATE_ON_DISCONNECT) : true;
  s_show_week = persist_exists(PERSIST_KEY_SHOW_WEEK)
                ? persist_read_bool(PERSIST_KEY_SHOW_WEEK) : true;
  s_slot_top_left = persist_exists(PERSIST_KEY_SLOT_TOP_LEFT)
                     ? persist_read_int(PERSIST_KEY_SLOT_TOP_LEFT) : SLOT_HEART;
  s_slot_top_right = persist_exists(PERSIST_KEY_SLOT_TOP_RIGHT)
                      ? persist_read_int(PERSIST_KEY_SLOT_TOP_RIGHT) : SLOT_STEPS;
  s_slot_bottom_left = persist_exists(PERSIST_KEY_SLOT_BOTTOM_LEFT)
                        ? persist_read_int(PERSIST_KEY_SLOT_BOTTOM_LEFT) : SLOT_CALORIES;
  s_slot_bottom_right = persist_exists(PERSIST_KEY_SLOT_BOTTOM_RIGHT)
                         ? persist_read_int(PERSIST_KEY_SLOT_BOTTOM_RIGHT) : SLOT_BATTERY;

  apply_palette(s_dark_mode);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_window, true);

  // Initial time display
  time_t now = time(NULL);
  update_time_display(localtime(&now));

  // Tick timer
  apply_tick_subscription();

  // Battery
  BatteryChargeState st = battery_state_service_peek();
  s_battery_percent = st.charge_percent;
  s_charging = st.is_charging;
  battery_state_service_subscribe(battery_handler);

  // Bluetooth
  s_bt_connected = connection_service_peek_pebble_app_connection();
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  // Quiet time
  s_quiet_time = quiet_time_is_active();
  layer_mark_dirty(s_qt_layer);

  // Accelerometer tap for shake-to-show-seconds
  accel_tap_service_subscribe(accel_tap_handler);

  // Backlight service for show-seconds-on-backlight
  backlight_service_subscribe(backlight_handler);

#if defined(PBL_HEALTH)
  if (health_service_events_subscribe(health_handler, NULL)) {
    refresh_health();
  }
#endif

  // AppMessage for Clay settings
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(128, 128);
}

static void deinit(void) {
  if (s_seconds_timer) {
    app_timer_cancel(s_seconds_timer);
  }
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  accel_tap_service_unsubscribe();
  backlight_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
