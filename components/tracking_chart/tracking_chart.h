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

/* Transparent, read-only chart showing a rolling window (default 20 seconds).
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

/* Sets the visible duration; zero is invalid and leaves the setting unchanged.
 * Applies immediately. Increasing the window cannot recover discarded samples.
 * Older actual samples are pruned on the next append. Default: 20000 ms. */
lv_result_t ui_tracking_chart_set_window_ms(lv_obj_t * obj, uint32_t window_ms);

/* Left-edge fade in pixels, capped to the plot width when drawing.
 * Zero disables fading. Applies immediately. Default: 48 px. */
void ui_tracking_chart_set_fade_width(lv_obj_t * obj, uint32_t width_px);

/* Copies unit text; decimals must be 0..6. Default: empty unit, one decimal. */
lv_result_t ui_tracking_chart_set_format(lv_obj_t * obj, const char * unit, uint8_t decimals);

/* Clears actual history, retaining target, range, formatting and window/fade settings. */
void ui_tracking_chart_reset(lv_obj_t * obj);

#ifdef __cplusplus
}
#endif

#endif /* TRACKING_CHART_H */
