#ifndef SEGMENTED_SWITCH_H
#define SEGMENTED_SWITCH_H
#include "lvgl/lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Two-option switch. Checked selects the left option. Default: ON/OFF, checked.
 * User changes emit LV_EVENT_VALUE_CHANGED; setters do not emit events.
 * LVGL size, styles and disabled state apply to the root object. */
lv_obj_t * segmented_switch_create(lv_obj_t * parent);
void segmented_switch_set_labels(lv_obj_t * obj, const char * left, const char * right);
void segmented_switch_set_checked(lv_obj_t * obj, bool checked, bool animate);
bool segmented_switch_get_checked(lv_obj_t * obj);

#ifdef __cplusplus
}
#endif
#endif
