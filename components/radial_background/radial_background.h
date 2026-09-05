#ifndef RADIAL_BACKGROUND_H
#define RADIAL_BACKGROUND_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

typedef struct {
    lv_color_t color;
    int32_t radius;
} radial_background_stop_t;

lv_obj_t * radial_background_create(lv_obj_t * parent);

/* Center in pixels relative to the object's top-left corner; default (0, 0). */
void radial_background_set_center(lv_obj_t * obj, int32_t x, int32_t y);

/* Copies count >= 1 stops with nonnegative, strictly increasing pixel radii.
 * Inside the first radius and outside the last, the endpoint colors are held.
 * Between radii, colors blend linearly. Invalid input/allocation failure leaves
 * the previous configuration intact. Count is not limited by LV_GRADIENT_MAX_STOPS.
 */
lv_result_t radial_background_set_stops(lv_obj_t * obj,
                                         const radial_background_stop_t * stops,
                                         uint16_t count);

#ifdef __cplusplus
}
#endif

#endif /* RADIAL_BACKGROUND_H */
