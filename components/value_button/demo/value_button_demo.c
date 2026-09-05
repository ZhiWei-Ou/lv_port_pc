#include "value_button/demo/value_button_demo.h"
#include "value_button/value_button.h"
#include <string.h>

static void toggle_value(lv_event_t * e)
{
    lv_obj_t * button = lv_event_get_current_target(e);
    value_button_set_value(button, strcmp(value_button_get_value(button), "ON") == 0 ? "OFF" : "ON");
}

void value_button_demo(void)
{
    lv_obj_t * screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x141414), 0);
    for(int i = 0; i < 3; i++) {
        lv_obj_t * button = value_button_create(screen);
        value_button_set_title(button, i == 0 ? "ROUTINE 1" : i == 1 ? "ROUTINE 2" : "ROUTINE 3");
        value_button_set_value(button, i == 0 ? "ON" : "OFF");
        lv_obj_align(button, LV_ALIGN_CENTER, 0, (i - 1) * 84);
        lv_obj_add_event_cb(button, toggle_value, LV_EVENT_CLICKED, NULL);
        if(i == 2) lv_obj_set_state(button, LV_STATE_DISABLED, true);
    }
}
