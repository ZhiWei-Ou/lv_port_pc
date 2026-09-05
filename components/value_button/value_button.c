#include "value_button/value_button.h"
#include "button/glass_button.h"

/* Labels are owned by this composite button in title/value order. */
lv_obj_t * value_button_create(lv_obj_t * parent)
{
    lv_obj_t * obj = glass_button_create(parent);
    lv_obj_set_size(obj, 320, 64);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x71695D), 0);
    lv_obj_set_style_bg_opa(obj, 65, 0);
    lv_obj_set_style_pad_left(obj, 24, 0);
    lv_obj_set_style_pad_right(obj, 24, 0);
    lv_obj_set_style_pad_column(obj, 12, 0);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t * title = lv_label_create(obj);
    lv_obj_set_flex_grow(title, 1);
    lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
    lv_label_set_text(title, "ROUTINE");
    lv_obj_set_clickable(title, false);
    lv_obj_t * value = lv_label_create(obj);
    lv_obj_set_width(value, LV_PCT(30));
    lv_label_set_long_mode(value, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(value, lv_color_hex(0xA29B91), 0);
    lv_label_set_text(value, "OFF");
    lv_obj_set_clickable(value, false);
    return obj;
}

void value_button_set_title(lv_obj_t * obj, const char * title)
{
    lv_label_set_text(lv_obj_get_child(obj, 0), title);
}

void value_button_set_value(lv_obj_t * obj, const char * value)
{
    lv_label_set_text(lv_obj_get_child(obj, 1), value);
}

const char * value_button_get_value(lv_obj_t * obj)
{
    return lv_label_get_text(lv_obj_get_child(obj, 1));
}
