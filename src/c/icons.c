// ---------------------------------------------------------------------------
// Icon primitives.
//
// Run tables are stored as static const int8_t arrays. Drawing uses
// graphics_fill_rect() with corner radius 0.
// ---------------------------------------------------------------------------

#include "icons.h"

// Stride-3 ([y, x, w]) — drawn as 1px horizontal scanlines.
static void draw_runs(GContext *ctx, GColor color,
                      const int8_t *runs, size_t count, int x, int y) {
  graphics_context_set_fill_color(ctx, color);
  for (size_t i = 0; i < count; i += 3) {
    GRect r = GRect(x + runs[i + 1], y + runs[i], runs[i + 2], 1);
    graphics_fill_rect(ctx, r, 0, GCornerNone);
  }
}

// Stride-4 ([y, x, w, h]) — filled rectangles.
static void draw_rects(GContext *ctx, GColor color,
                       const int8_t *rects, size_t count, int x, int y) {
  graphics_context_set_fill_color(ctx, color);
  for (size_t i = 0; i < count; i += 4) {
    GRect r = GRect(x + rects[i + 1], y + rects[i], rects[i + 2], rects[i + 3]);
    graphics_fill_rect(ctx, r, 0, GCornerNone);
  }
}

// ---- Heart (22w x 18h, ECG line punched in bgColor) ----------------------

static const int8_t HEART_RUNS[] = {
   0,  4, 3,   0, 15, 3,
   1,  2, 7,   1, 13, 7,
   2,  1, 9,   2, 12, 9,
   3,  0, 11,  3, 11, 11,
   4,  0, 11,  4, 11, 11,
   5,  0, 22,
   6,  0, 22,
   7,  1, 20,
   8,  2, 18,
   9,  3, 16,
  10,  4, 14,
  11,  5, 12,
  12,  6, 10,
  13,  7,  8,
  14,  8,  6,
  15,  9,  4,
  16, 10,  2,
  17, 10,  2,
};

static const int8_t HEART_EKG[] = {
  11,  1, 6, 1,
  10,  7, 1, 1,
   9,  8, 1, 1,
   8,  9, 1, 1,
   7, 10, 1, 1,
   8, 11, 1, 1,
   9, 12, 1, 1,
  10, 13, 1, 1,
  11, 14, 1, 1,
  12, 15, 1, 1,
  11, 16, 6, 1,
};

void icons_draw_heart(GContext *ctx, int x, int y, GColor color, GColor bgColor) {
  x += 1; y += 3; // centre within 24x24 box
  draw_runs(ctx, color, HEART_RUNS, sizeof(HEART_RUNS), x, y);
  draw_rects(ctx, bgColor, HEART_EKG, sizeof(HEART_EKG), x, y);
}

// ---- Steps (20w x 18h, two footprints) -----------------------------------

static const int8_t BIG_FOOT_PAD[] = {
   0,  2, 3,
   1,  1, 5,
   2,  0, 7,
   3,  0, 7,
   4,  0, 7,
   5,  1, 5,
   6,  1, 5,
   7,  0, 7,
   8,  0, 7,
   9,  0, 7,
  10,  0, 7,
  11,  1, 5,
  12,  2, 3,
};

static const int8_t BIG_FOOT_TOES[] = {
  -4, 0, 2, 2,
  -5, 3, 2, 2,
  -4, 6, 2, 2,
};

static const int8_t SMALL_FOOT_PAD[] = {
   0, 1, 3,
   1, 0, 5,
   2, 0, 5,
   3, 0, 5,
   4, 1, 3,
   5, 0, 5,
   6, 0, 5,
   7, 0, 5,
   8, 1, 3,
};

static const int8_t SMALL_FOOT_TOES[] = {
  -3, 0, 2, 2,
  -4, 2, 2, 2,
  -3, 4, 2, 2,
};

void icons_draw_steps(GContext *ctx, int x, int y, GColor color) {
  y += 1;
  int bx = x + 14, by = y + 7;
  draw_runs(ctx, color, BIG_FOOT_PAD, sizeof(BIG_FOOT_PAD), bx, by);
  draw_rects(ctx, color, BIG_FOOT_TOES, sizeof(BIG_FOOT_TOES), bx, by);
  int sx = x + 2, sy = y + 11;
  draw_runs(ctx, color, SMALL_FOOT_PAD, sizeof(SMALL_FOOT_PAD), sx, sy);
  draw_rects(ctx, color, SMALL_FOOT_TOES, sizeof(SMALL_FOOT_TOES), sx, sy);
}

// ---- Flame (14w x 23h) ---------------------------------------------------

static const int8_t FLAME_RUNS[] = {
   0,  7,  1,
   1,  6,  2,
   2,  5,  3,
   3,  5,  4,
   4,  4,  5,
   5,  3,  6,
   6,  3,  7,
   7,  2,  8,
   8,  2,  9,
   9,  1, 10,
  10,  1, 11,
  11,  1, 12,
  12,  0, 13,
  13,  0, 14,
  14,  0, 14,
  15,  0, 14,
  16,  1, 12,
  17,  1, 12,
  18,  2, 10,
  19,  3,  8,
  20,  4,  6,
  21,  5,  4,
  22,  6,  2,
};

static const int8_t FLAME_INNER[] = {
  10, 6, 2,
  11, 6, 3,
  12, 5, 4,
  13, 5, 5,
  14, 4, 6,
  15, 4, 6,
  16, 5, 5,
  17, 6, 3,
};

void icons_draw_flame(GContext *ctx, int x, int y, GColor color, GColor bgColor) {
  x += 5;
  draw_runs(ctx, color, FLAME_RUNS, sizeof(FLAME_RUNS), x, y);
  draw_runs(ctx, bgColor, FLAME_INNER, sizeof(FLAME_INNER), x, y);
}

// ---- Battery -------------------------------------------------------------

void icons_draw_battery(GContext *ctx, int x, int y,
                        GColor outline, GColor bg, GColor fill,
                        int percent, bool charging) {
  x += 5; // centre 13w body within 24w box

  graphics_context_set_fill_color(ctx, outline);
  graphics_fill_rect(ctx, GRect(x + 4,  y + 0,  5,  1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 1,  y + 1,  11, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 0,  y + 2,  13, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 0,  y + 3,  1,  18), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 12, y + 3,  1,  18), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 0,  y + 21, 13, 2), 0, GCornerNone);

  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, GRect(x + 1, y + 3, 11, 18), 0, GCornerNone);

  if (percent > 0) {
    int fillH = (16 * percent) / 100;
    if (fillH < 1) fillH = 1;
    graphics_context_set_fill_color(ctx, fill);
    graphics_fill_rect(ctx, GRect(x + 2, y + 20 - fillH, 9, fillH), 0, GCornerNone);
  }

  if (charging) {
    graphics_context_set_fill_color(ctx, outline);
    static const int8_t bolt[] = {
      5, 8, 3, 1,
      6, 7, 3, 1,
      7, 5, 5, 1,
      8, 4, 6, 1,
      9, 4, 6, 1,
      10, 5, 4, 1,
      11, 4, 4, 1,
      12, 3, 4, 1,
      13, 3, 3, 1,
      14, 3, 2, 1,
    };
    for (size_t i = 0; i < sizeof(bolt); i += 4) {
      graphics_fill_rect(ctx,
        GRect(x + bolt[i+1], y + bolt[i], bolt[i+2], bolt[i+3]),
        0, GCornerNone);
    }
  }
}
