#ifndef TRACKING_CHART_H
#define TRACKING_CHART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

typedef struct {
    uint32_t time_ms;
    float value;
} ui_tracking_chart_sample_t;

/* Transparent, read-only chart showing the latest 20 seconds at a fixed scale.
 * Time advances only when a sample is appended. */
lv_obj_t * ui_tracking_chart_create(lv_obj_t * parent);

/* Copies >= 2 finite samples, beginning at zero with strictly increasing times.
 * The final time defines duration. Success resets actual history and fixes the
 * shared Y range from the complete target plan, including zero and 10% padding.
 * Invalid input/allocation failure leaves the previous state intact. */
lv_result_t ui_tracking_chart_set_target(lv_obj_t * obj,
                                         const ui_tracking_chart_sample_t * samples,
                                         uint16_t count);

/* Requires a target; finite values and strictly increasing times <= duration.
 * The first sample may be at zero. Older history is discarded, retaining only
 * the preceding points needed to interpolate the window boundary. */
lv_result_t ui_tracking_chart_append_actual(lv_obj_t * obj, uint32_t time_ms, float value);

/* Copies unit text; decimals must be 0..6. Default: empty unit, one decimal. */
lv_result_t ui_tracking_chart_set_format(lv_obj_t * obj, const char * unit, uint8_t decimals);

/* Clears actual history, retaining target, range and formatting. */
void ui_tracking_chart_reset(lv_obj_t * obj);

#ifdef __cplusplus
}
#endif

#endif /* TRACKING_CHART_H */
