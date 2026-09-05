#include "segmented_switch/demo/segmented_switch_demo.h"
#include "segmented_switch/segmented_switch.h"

void segmented_switch_demo(void)
{
    lv_obj_t * screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x141414), 0);
    lv_obj_t * status = segmented_switch_create(screen);
    lv_obj_align(status, LV_ALIGN_CENTER, 0, -50);
    lv_obj_t * schedule = segmented_switch_create(screen);
    segmented_switch_set_labels(schedule, "ANYDAY", "SINGLE DAY");
    segmented_switch_set_checked(schedule, false, false);
    lv_obj_align(schedule, LV_ALIGN_CENTER, 0, 40);
}
