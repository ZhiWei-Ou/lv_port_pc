#include "glow/glow.h"

#include <math.h>

#include "lvgl/src/core/lv_obj_class_private.h"
#include "lvgl/src/core/lv_obj_private.h"

typedef struct {
    lv_obj_t obj;
    lv_obj_t * layer;
    lv_grad_dsc_t gradient;
    int32_t center_x;
    int32_t center_y;
    uint16_t eccentricity;
    uint16_t angle;
    uint16_t spread;
} ui_glow_t;

static void glow_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void glow_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void glow_refresh(ui_glow_t * glow);
static void glow_set_spread(ui_glow_t * glow, uint16_t spread, bool cancel_animation);
static void glow_spread_anim_cb(void * var, int32_t value);

static const lv_obj_class_t ui_glow_class = {
    .constructor_cb = glow_constructor,
    .event_cb = glow_event,
    .width_def = LV_PCT(100),
    .height_def = LV_PCT(100),
    .instance_size = sizeof(ui_glow_t),
    .base_class = &lv_obj_class,
    .name = "ui_glow",
};

lv_obj_t * ui_glow_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&ui_glow_class, parent);
    lv_obj_class_init_obj(obj);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    return obj;
}

void ui_glow_set_center(lv_obj_t * obj, int32_t offset_x, int32_t offset_y)
{
    ui_glow_t * glow = (ui_glow_t *)obj;
    glow->center_x = offset_x;
    glow->center_y = offset_y;
    glow_refresh(glow);
}

void ui_glow_set_color(lv_obj_t * obj, lv_color_t color)
{
    ui_glow_t * glow = (ui_glow_t *)obj;
    for(uint8_t i = 0; i < glow->gradient.stops_count; i++) {
        glow->gradient.stops[i].color = color;
    }
    glow_refresh(glow);
}

lv_result_t ui_glow_set_gradient(lv_obj_t * obj, const ui_glow_gradient_stop_t * stops, uint8_t stop_count)
{
    if(stops == NULL || stop_count < 2 || stop_count > LV_GRADIENT_MAX_STOPS) {
        return LV_RESULT_INVALID;
    }

    for(uint8_t i = 1; i < stop_count; i++) {
        if(stops[i - 1].position > stops[i].position) {
            return LV_RESULT_INVALID;
        }
    }

    ui_glow_t * glow = (ui_glow_t *)obj;
    for(uint8_t i = 0; i < stop_count; i++) {
        glow->gradient.stops[i].color = stops[i].color;
        glow->gradient.stops[i].opa = stops[i].opacity;
        glow->gradient.stops[i].frac = stops[i].position;
    }
    glow->gradient.stops_count = stop_count;
    glow_refresh(glow);
    return LV_RESULT_OK;
}

void ui_glow_set_eccentricity(lv_obj_t * obj, uint16_t eccentricity)
{
    ui_glow_t * glow = (ui_glow_t *)obj;
    glow->eccentricity = LV_MIN(eccentricity, 999);
    glow_refresh(glow);
}

void ui_glow_set_angle(lv_obj_t * obj, int16_t degrees)
{
    ui_glow_t * glow = (ui_glow_t *)obj;
    glow->angle = (uint16_t)((degrees % 360 + 360) % 360);
    glow_refresh(glow);
}

void ui_glow_set_spread(lv_obj_t * obj, uint16_t spread)
{
    glow_set_spread((ui_glow_t *)obj, spread, true);
}

void ui_glow_animate_spread(lv_obj_t * obj, uint16_t target_spread, uint32_t duration_ms, lv_anim_path_cb_t easing)
{
    ui_glow_t * glow = (ui_glow_t *)obj;
    target_spread = LV_MIN(target_spread, 1000);
    lv_anim_delete(obj, glow_spread_anim_cb);

    if(duration_ms == 0) {
        glow_set_spread(glow, target_spread, false);
        return;
    }

    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, obj);
    lv_anim_set_values(&animation, glow->spread, target_spread);
    lv_anim_set_duration(&animation, duration_ms);
    lv_anim_set_exec_cb(&animation, glow_spread_anim_cb);
    lv_anim_set_path_cb(&animation, easing != NULL ? easing : lv_anim_path_ease_in_out);
    lv_anim_start(&animation);
}

static void glow_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    ui_glow_t * glow = (ui_glow_t *)obj;

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_scrollable(obj, false);
    lv_obj_set_overflow_visible(obj, false);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);

    lv_color_t colors[] = {lv_color_white(), lv_color_white()};
    lv_opa_t opacities[] = {LV_OPA_70, LV_OPA_TRANSP};
    uint8_t positions[] = {0, 255};
    lv_grad_init_stops(&glow->gradient, colors, opacities, positions, 2);

    glow->layer = lv_obj_create(obj);
    lv_obj_set_clickable(glow->layer, false);
    lv_obj_set_ignore_layout(glow->layer, true);
    lv_obj_set_scrollable(glow->layer, false);
    lv_obj_set_style_border_width(glow->layer, 0, 0);
    lv_obj_set_style_pad_all(glow->layer, 0, 0);
    lv_obj_set_style_radius(glow->layer, 0, 0);
    lv_obj_set_style_bg_opa(glow->layer, LV_OPA_COVER, 0);
    lv_obj_move_background(glow->layer);

    glow->spread = 1000;
    glow_refresh(glow);
}

static void glow_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    if(lv_obj_event_base(class_p, e) != LV_RESULT_OK) {
        return;
    }

    if(lv_event_get_code(e) == LV_EVENT_SIZE_CHANGED) {
        glow_refresh((ui_glow_t *)lv_event_get_current_target(e));
    }
}

static void glow_refresh(ui_glow_t * glow)
{
    int32_t width = lv_obj_get_width(&glow->obj);
    int32_t height = lv_obj_get_height(&glow->obj);

    /* A transformed translucent layer cannot restore pixels from its previous
     * position by itself. Repaint the parent so the backdrop is restored first. */
    lv_obj_t * parent = lv_obj_get_parent(&glow->obj);
    if(parent != NULL) {
        lv_obj_invalidate(parent);
    }

    if(glow->spread == 0 || width <= 0 || height <= 0) {
        lv_obj_set_hidden(glow->layer, true);
        return;
    }

    int32_t long_side = LV_MAX(width, height);
    int32_t diameter = (long_side * glow->spread + 500) / 1000;
    diameter = LV_MAX(diameter, 1);
    int32_t radius = diameter / 2;
    float eccentricity = glow->eccentricity / 1000.0f;
    int32_t scale_y = (int32_t)(sqrtf(1.0f - eccentricity * eccentricity) * 256.0f + 0.5f);

    lv_grad_radial_init(&glow->gradient, radius, radius, diameter, radius, LV_GRAD_EXTEND_PAD);
    lv_obj_set_style_bg_grad(glow->layer, &glow->gradient, 0);
    lv_obj_set_size(glow->layer, diameter, diameter);
    lv_obj_set_pos(glow->layer,
                   width / 2 + glow->center_x - radius,
                   height / 2 + glow->center_y - radius);
    lv_obj_set_style_transform_pivot_x(glow->layer, radius, 0);
    lv_obj_set_style_transform_pivot_y(glow->layer, radius, 0);
    lv_obj_set_style_transform_scale_x(glow->layer, 256, 0);
    lv_obj_set_style_transform_scale_y(glow->layer, scale_y, 0);
    lv_obj_set_style_transform_rotation(glow->layer, glow->angle * 10, 0);
    lv_obj_set_hidden(glow->layer, false);
    lv_obj_invalidate(glow->layer);
}

static void glow_set_spread(ui_glow_t * glow, uint16_t spread, bool cancel_animation)
{
    if(cancel_animation) {
        lv_anim_delete(&glow->obj, glow_spread_anim_cb);
    }
    glow->spread = LV_MIN(spread, 1000);
    glow_refresh(glow);
}

static void glow_spread_anim_cb(void * var, int32_t value)
{
    glow_set_spread((ui_glow_t *)var, (uint16_t)LV_CLAMP(0, value, 1000), false);
}
