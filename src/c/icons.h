#pragma once
#include <pebble.h>

// All icons fit inside a 24x24 bounding box. The icon's content is centred
// within that box. Caller passes the top-left of the box.

#define ICON_BOX_W 24
#define ICON_BOX_H 24

// Each icon takes the GContext to draw into, plus the box position and a
// fill colour. bgColor is the "background" colour used to punch cutouts
// (EKG line, inner flame) — pass GColorBlack for the standard layout.

void icons_draw_heart(GContext *ctx, int x, int y, GColor color, GColor bgColor);
void icons_draw_steps(GContext *ctx, int x, int y, GColor color);
void icons_draw_flame(GContext *ctx, int x, int y, GColor color, GColor bgColor);
void icons_draw_battery(GContext *ctx, int x, int y,
                        GColor outline, GColor bg, GColor fill,
                        int percent, bool charging);
