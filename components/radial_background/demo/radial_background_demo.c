#include "radial_background/demo/radial_background_demo.h"
#include "radial_background/radial_background.h"

void radial_background_demo(void)
{
    const radial_background_stop_t stops[] = {
        {lv_color_hex(0xAD4902), 39},
        {lv_color_hex(0x510900), 233},
        {lv_color_hex(0x141414), 467},
    };

    lv_obj_t * background = radial_background_create(lv_screen_active());
    radial_background_set_center(background, 350, 350);
    if(radial_background_set_stops(background, stops, LV_ARRAYLEN(stops)) != LV_RESULT_OK) {
        LV_LOG_ERROR("Failed to configure radial background");
    }
}
