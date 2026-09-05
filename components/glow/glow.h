#ifndef GLOW_H
#define GLOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

typedef struct {
    lv_color_t color;
    lv_opa_t opacity;
    uint8_t position;
} glow_gradient_stop_t;

lv_obj_t * glow_create(lv_obj_t * parent);

void glow_set_center(lv_obj_t * obj, int32_t offset_x, int32_t offset_y);
void glow_set_color(lv_obj_t * obj, lv_color_t color);

lv_result_t glow_set_gradient(
    lv_obj_t * obj,
    const glow_gradient_stop_t * stops,
    uint8_t stop_count
);

void glow_set_eccentricity(lv_obj_t * obj, uint16_t eccentricity);
void glow_set_angle(lv_obj_t * obj, int16_t degrees);
void glow_set_spread(lv_obj_t * obj, uint16_t spread);

void glow_animate_spread(
    lv_obj_t * obj,
    uint16_t target_spread,
    uint32_t duration_ms,
    lv_anim_path_cb_t easing
);

#ifdef __cplusplus
}
#endif

#endif /* GLOW_H */
