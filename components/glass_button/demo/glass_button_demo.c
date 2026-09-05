#include "glass_button/demo/glass_button_demo.h"

#include "glass_button/glass_button.h"
#include "radial_background/demo/radial_background_demo.h"

typedef struct {
    bool dragging;
    lv_point_t grab_point;
    lv_point_t start_position;
    lv_obj_t * label;
} drag_state_t;

static void drag_event(lv_event_t * e)
{
    lv_obj_t * button = lv_event_get_current_target(e);
    drag_state_t * state = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_DELETE) {
        lv_free(state);
    }
    else if(code == LV_EVENT_LONG_PRESSED) {
        lv_indev_t * indev = lv_event_get_indev(e);
        if(lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            state->dragging = true;
            lv_indev_get_point(indev, &state->grab_point);
            state->start_position.x = lv_obj_get_x(button);
            state->start_position.y = lv_obj_get_y(button);
            lv_label_set_text(state->label, "Drag to move");
            /* Keep the grab when the pointer moves faster than the button. */
            lv_obj_set_press_lock(button, true);
        }
    }
    else if(code == LV_EVENT_PRESSING && state->dragging) {
        lv_point_t point;
        lv_indev_get_point(lv_event_get_indev(e), &point);
        lv_obj_t * parent = lv_obj_get_parent(button);
        int32_t max_x = LV_MAX(0, lv_obj_get_content_width(parent) - lv_obj_get_width(button));
        int32_t max_y = LV_MAX(0, lv_obj_get_content_height(parent) - lv_obj_get_height(button));
        /* SDL can deliver multiple reads before layout updates object coordinates.
         * Compute from the grab origin so repeated reads cannot undo a move. */
        lv_obj_set_pos(button,
                       LV_CLAMP(0, state->start_position.x + point.x - state->grab_point.x, max_x),
                       LV_CLAMP(0, state->start_position.y + point.y - state->grab_point.y, max_y));
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        state->dragging = false;
        lv_label_set_text(state->label, "Hold to drag");
        lv_obj_set_press_lock(button, false);
    }
}

void glass_button_demo(void)
{
    radial_background_demo();
    lv_obj_t * screen = lv_screen_active();
    lv_obj_update_layout(screen);

    lv_obj_t * button = ui_glass_button_create(screen);
    lv_obj_set_size(button, 320, 72);
    lv_obj_set_pos(button, (lv_obj_get_width(screen) - 320) / 2,
                   (lv_obj_get_height(screen) - 72) / 2);

    drag_state_t * state = lv_malloc_zeroed(sizeof(*state));
    if(state == NULL) {
        LV_LOG_ERROR("Failed to allocate button drag state");
        lv_obj_delete(button);
        return;
    }
    state->label = lv_label_create(button);
    lv_label_set_text(state->label, "Hold to drag");
    lv_obj_center(state->label);
    lv_obj_add_event_cb(button, drag_event, LV_EVENT_ALL, state);
}
