#ifndef VALUE_BUTTON_H
#define VALUE_BUTTON_H
#include "lvgl/lvgl.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Capsule button with a left title and right value. Text is copied.
 * Use LV_EVENT_CLICKED and standard LVGL size, style and state APIs. */
lv_obj_t * value_button_create(lv_obj_t * parent);
void value_button_set_title(lv_obj_t * obj, const char * title);
void value_button_set_value(lv_obj_t * obj, const char * value);
const char * value_button_get_value(lv_obj_t * obj);

#ifdef __cplusplus
}
#endif
#endif
