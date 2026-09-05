#include "glass_button/glass_button.h"

#include <math.h>

#include "lvgl/src/core/lv_obj_class_private.h"
#include "lvgl/src/core/lv_obj_event_private.h"
#include "lvgl/src/widgets/button/lv_button_private.h"

#define GLASS_BUTTON_SCALE_NORMAL 256
#define GLASS_BUTTON_SCALE_PRESSED 243 /* 94.92%, nearest LVGL scale to 95%. */

static void glass_button_event(const lv_obj_class_t * class_p, lv_event_t * e);

static const lv_obj_class_t glass_button_class = {
    .event_cb = glass_button_event,
    .width_def = 64,
    .height_def = 64,
    .instance_size = sizeof(lv_button_t),
    .base_class = &lv_button_class,
    .name = "glass_button",
};

static const lv_style_prop_t press_properties[] = {
    LV_STYLE_TRANSFORM_SCALE_X,
    LV_STYLE_TRANSFORM_SCALE_Y,
    LV_STYLE_PROP_INV,
};

static const lv_style_transition_dsc_t press_transition = {
    .props = press_properties,
    .path_xcb = lv_anim_path_ease_in_out,
    .time = 140,
};

static const lv_style_transition_dsc_t release_transition = {
    .props = press_properties,
    .path_xcb = lv_anim_path_ease_in_out,
    .time = 180,
};

lv_obj_t * glass_button_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&glass_button_class, parent);
    lv_obj_class_init_obj(obj);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 64, 64);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xFFE8D5), 0);
    lv_obj_set_style_bg_opa(obj, 16, 0);
    lv_obj_set_style_border_color(obj, lv_color_white(), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_opa(obj, 24, 0);
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    lv_obj_set_style_transform_pivot_x(obj, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(obj, LV_PCT(50), 0);
    lv_obj_set_style_transform_scale(obj, GLASS_BUTTON_SCALE_NORMAL, 0);
    lv_obj_set_style_transform_scale(obj, GLASS_BUTTON_SCALE_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_transition(obj, &release_transition, 0);
    lv_obj_set_style_transition(obj, &press_transition, LV_STATE_PRESSED);
    lv_obj_set_style_outline_color(obj, lv_color_white(), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_opa(obj, LV_OPA_50, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(obj, 1, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_pad(obj, 3, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_opa(obj, LV_OPA_40, LV_STATE_DISABLED);
    lv_obj_set_adv_hittest(obj, true);
    lv_obj_set_press_lock(obj, false);
    return obj;
}

static void glass_button_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    if(lv_obj_event_base(class_p, e) != LV_RESULT_OK) {
        return;
    }
    lv_event_code_t code = lv_event_get_code(e);
    if(code != LV_EVENT_DRAW_MAIN && code != LV_EVENT_HIT_TEST) {
        return;
    }

    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    int32_t radius = LV_MIN(lv_area_get_width(&area), lv_area_get_height(&area)) / 2;
    if(code == LV_EVENT_HIT_TEST) {
        lv_hit_test_info_t * hit = lv_event_get_hit_test_info(e);
        int64_t dx = hit->point->x - LV_CLAMP(area.x1 + radius, hit->point->x, area.x2 - radius + 1);
        int64_t dy = hit->point->y - LV_CLAMP(area.y1 + radius, hit->point->y, area.y2 - radius + 1);
        hit->res = dx * dx + dy * dy <= (int64_t)radius * radius;
        return;
    }

    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.color = lv_color_white();
    arc.width = 1;
    arc.radius = radius;

    /* Anchor reflections to opposite rounded corners, including on capsules. */
    for(int side = 0; side < 2; side++) {
        arc.center.x = side == 0 ? area.x1 + radius : area.x2 - radius + 1;
        arc.center.y = side == 0 ? area.y1 + radius : area.y2 - radius + 1;
        for(int segment = 0; segment < 30; segment++) {
            float t = (segment + 0.5f) / 30.0f;
            float strength = sinf(t * 3.14159265f);
            arc.opa = (lv_opa_t)((side == 0 ? 85 : 55) * strength * strength);
            arc.start_angle = (side == 0 ? 180 : 0) + segment * 3;
            arc.end_angle = arc.start_angle + 3;
            lv_draw_arc(lv_event_get_layer(e), &arc);
        }
    }
}
