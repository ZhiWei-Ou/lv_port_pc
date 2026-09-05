#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "segmented_switch/segmented_switch.h"
#include "value_button/value_button.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while(0)
static unsigned changes;
static unsigned clicks;
static unsigned char pixels[700 * 700 * 4];
static void flush(lv_display_t * display, const lv_area_t * area, uint8_t * data)
{
    LV_UNUSED(area); LV_UNUSED(data);
    lv_display_flush_ready(display);
}
static void changed(lv_event_t * e) { LV_UNUSED(e); changes++; }
static void clicked(lv_event_t * e) { LV_UNUSED(e); clicks++; }
static void advance(uint32_t ms)
{
    lv_tick_inc(ms);
    lv_timer_handler();
    lv_refr_now(NULL);
}
int main(int argc, char ** argv)
{
    lv_init();
    lv_display_t * display = lv_display_create(700, 700);
    lv_display_set_buffers(display, pixels, NULL, sizeof(pixels), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, flush);
    lv_obj_t * control = segmented_switch_create(lv_screen_active());
    lv_obj_add_event_cb(control, changed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_update_layout(control);
    CHECK(segmented_switch_get_checked(control));
    lv_obj_t * thumb = lv_obj_get_child(control, 0);
    int32_t left = lv_obj_get_x(thumb);
    lv_obj_send_event(control, LV_EVENT_CLICKED, NULL);
    CHECK(!segmented_switch_get_checked(control) && changes == 1);
    advance(100);
    int32_t middle = lv_obj_get_x(thumb);
    CHECK(middle > left && middle < 160);
    lv_obj_send_event(control, LV_EVENT_CLICKED, NULL);
    CHECK(segmented_switch_get_checked(control) && changes == 2);
    CHECK(lv_obj_get_x(thumb) == middle);
    advance(240);
    CHECK(lv_obj_get_x(thumb) == left);
    uint32_t key = LV_KEY_RIGHT;
    lv_obj_send_event(control, LV_EVENT_KEY, &key);
    advance(240);
    CHECK(!segmented_switch_get_checked(control) && changes == 3);
    lv_obj_set_size(control, 400, 72);
    lv_obj_update_layout(control);
    CHECK(lv_obj_get_x(thumb) > 190);
    lv_obj_set_state(control, LV_STATE_DISABLED, true);
    lv_obj_send_event(control, LV_EVENT_CLICKED, NULL);
    CHECK(changes == 3);
    segmented_switch_set_checked(control, true, true);
    CHECK(changes == 3);
    lv_obj_delete(control);
    advance(250);

    lv_obj_t * button = value_button_create(lv_screen_active());
    char title[] = "ROUTINE 1";
    value_button_set_title(button, title);
    title[0] = 'X';
    CHECK(strcmp(lv_label_get_text(lv_obj_get_child(button, 0)), "ROUTINE 1") == 0);
    value_button_set_value(button, "ON");
    CHECK(strcmp(value_button_get_value(button), "ON") == 0);
    lv_obj_add_event_cb(button, clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_send_event(button, LV_EVENT_CLICKED, NULL);
    CHECK(clicks == 1);
    value_button_set_title(button, "A very long routine title that must be truncated");
    value_button_set_value(button, "A long value");
    lv_obj_set_width(button, 220);
    lv_obj_update_layout(button);
    lv_area_t title_area, value_area;
    lv_obj_get_coords(lv_obj_get_child(button, 0), &title_area);
    lv_obj_get_coords(lv_obj_get_child(button, 1), &value_area);
    CHECK(title_area.x2 < value_area.x1);
    advance(20);
    lv_obj_delete(button);
    if(argc > 1) {
        lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x141414), 0);
        control = segmented_switch_create(lv_screen_active());
        lv_obj_align(control, LV_ALIGN_CENTER, 0, -90);
        button = value_button_create(lv_screen_active());
        value_button_set_title(button, "ROUTINE 1");
        value_button_set_value(button, "ON");
        lv_obj_center(button);
        lv_obj_t * second = value_button_create(lv_screen_active());
        value_button_set_title(second, "ROUTINE 2");
        lv_obj_align(second, LV_ALIGN_CENTER, 0, 84);
        advance(20);
        FILE * file = fopen(argv[1], "wb");
        CHECK(file != NULL);
        fprintf(file, "P6\n700 700\n255\n");
        for(size_t i = 0; i < 700 * 700; i++) {
            uint8_t rgb[] = {pixels[i * 4 + 2], pixels[i * 4 + 1], pixels[i * 4]};
            CHECK(fwrite(rgb, 1, 3, file) == 3);
        }
        CHECK(fclose(file) == 0);
    }
    lv_display_delete(display);
    lv_deinit();
    puts("Control interaction and animation checks passed");
}
