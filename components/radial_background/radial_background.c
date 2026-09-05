#include "radial_background/radial_background.h"

#include "lvgl/src/core/lv_obj_class_private.h"
#include "lvgl/src/core/lv_obj_private.h"

typedef struct {
    lv_obj_t obj;
    radial_background_stop_t * stops;
    uint16_t count;
    int32_t center_x;
    int32_t center_y;
} radial_background_t;

static void background_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void background_event(const lv_obj_class_t * class_p, lv_event_t * e);

static const lv_obj_class_t radial_background_class = {
    .destructor_cb = background_destructor,
    .event_cb = background_event,
    .width_def = LV_PCT(100),
    .height_def = LV_PCT(100),
    .instance_size = sizeof(radial_background_t),
    .base_class = &lv_obj_class,
    .name = "radial_background",
};

lv_obj_t * radial_background_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&radial_background_class, parent);
    lv_obj_class_init_obj(obj);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_clickable(obj, false);
    lv_obj_set_scrollable(obj, false);
    lv_obj_move_background(obj);
    return obj;
}

void radial_background_set_center(lv_obj_t * obj, int32_t x, int32_t y)
{
    radial_background_t * background = (radial_background_t *)obj;
    background->center_x = x;
    background->center_y = y;
    lv_obj_invalidate(obj);
}

lv_result_t radial_background_set_stops(lv_obj_t * obj,
                                         const radial_background_stop_t * stops,
                                         uint16_t count)
{
    if(stops == NULL || count == 0) {
        return LV_RESULT_INVALID;
    }
    for(uint16_t i = 0; i < count; i++) {
        if(stops[i].radius < 0 || (i > 0 && stops[i].radius <= stops[i - 1].radius)) {
            return LV_RESULT_INVALID;
        }
    }

    size_t size = sizeof(*stops) * count;
    radial_background_stop_t * copy = lv_malloc(size);
    if(copy == NULL) {
        return LV_RESULT_INVALID;
    }
    lv_memcpy(copy, stops, size);
    radial_background_t * background = (radial_background_t *)obj;
    lv_free(background->stops);
    background->stops = copy;
    background->count = count;
    lv_obj_invalidate(obj);
    return LV_RESULT_OK;
}

static void background_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    lv_free(((radial_background_t *)obj)->stops);
}

static void background_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    if(lv_obj_event_base(class_p, e) != LV_RESULT_OK) {
        return;
    }
    if(lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) {
        return;
    }

    lv_obj_t * obj = lv_event_get_current_target(e);
    radial_background_t * background = (radial_background_t *)obj;
    if(background->count == 0) {
        return;
    }

    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = lv_obj_get_style_opa_recursive(obj, LV_PART_MAIN);
    dsc.bg_color = background->stops[background->count - 1].color;
    lv_draw_rect(layer, &dsc, &area);

    /* Paint inward: each underlying layer is solid within the next radius,
     * so fading this color reveals exactly the next color in the sequence. */
    for(int32_t i = background->count - 2; i >= 0; i--) {
        lv_color_t colors[] = {background->stops[i].color, background->stops[i].color};
        lv_opa_t opacities[] = {LV_OPA_COVER, LV_OPA_TRANSP};
        lv_grad_init_stops(&dsc.bg_grad, colors, opacities, NULL, 2);
        lv_grad_radial_init(&dsc.bg_grad, background->center_x, background->center_y,
                            background->center_x + background->stops[i + 1].radius,
                            background->center_y, LV_GRAD_EXTEND_PAD);
        lv_grad_radial_set_focal(&dsc.bg_grad, background->center_x, background->center_y,
                                 background->stops[i].radius);
        lv_draw_rect(layer, &dsc, &area);
    }
}
