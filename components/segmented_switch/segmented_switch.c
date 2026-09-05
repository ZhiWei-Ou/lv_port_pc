#include "segmented_switch/segmented_switch.h"
#include "lvgl/src/core/lv_obj_private.h"
#include "lvgl/src/core/lv_obj_class_private.h"

typedef struct {
    lv_obj_t obj;
    lv_obj_t * thumb;
    lv_obj_t * left;
    lv_obj_t * right;
    int32_t position;
} segmented_switch_t;

static void layout(segmented_switch_t * control)
{
    if(control->thumb == NULL) return;
    int32_t width = LV_MAX(0, lv_obj_get_width(&control->obj) - 8);
    int32_t height = LV_MAX(0, lv_obj_get_height(&control->obj) - 8);
    lv_obj_set_size(control->thumb, width / 2, height);
    lv_obj_set_pos(control->thumb, 4 + (width - width / 2) * control->position / 1000, 4);
    lv_obj_set_width(control->left, width / 2);
    lv_obj_set_width(control->right, width - width / 2);
    lv_obj_align(control->left, LV_ALIGN_LEFT_MID, 4, 0);
    lv_obj_align(control->right, LV_ALIGN_RIGHT_MID, -4, 0);
}

static void animate_position(void * obj, int32_t value)
{
    segmented_switch_t * control = obj;
    control->position = value;
    layout(control);
}

void segmented_switch_set_checked(lv_obj_t * obj, bool checked, bool animate)
{
    segmented_switch_t * control = (segmented_switch_t *)obj;
    lv_obj_set_state(obj, LV_STATE_CHECKED, checked);
    lv_anim_delete(obj, animate_position);
    int32_t target = checked ? 0 : 1000;
    if(!animate) {
        animate_position(obj, target);
        return;
    }
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, obj);
    lv_anim_set_exec_cb(&animation, animate_position);
    lv_anim_set_values(&animation, control->position, target);
    lv_anim_set_duration(&animation, 220);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
    lv_anim_start(&animation);
}

bool segmented_switch_get_checked(lv_obj_t * obj)
{
    return lv_obj_has_state(obj, LV_STATE_CHECKED);
}

static void control_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    if(lv_obj_event_base(class_p, e) != LV_RESULT_OK) return;
    lv_obj_t * obj = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SIZE_CHANGED) layout((segmented_switch_t *)obj);
    if(lv_obj_has_state(obj, LV_STATE_DISABLED)) return;
    bool checked = segmented_switch_get_checked(obj);
    bool next = checked;
    if(code == LV_EVENT_CLICKED) {
        lv_indev_t * indev = lv_event_get_indev(e);
        if(indev && lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            lv_point_t point;
            lv_area_t area;
            lv_indev_get_point(indev, &point);
            lv_obj_get_coords(obj, &area);
            next = point.x < (area.x1 + area.x2 + 1) / 2;
        }
        else next = !checked;
    }
    else if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(key == LV_KEY_LEFT) next = true;
        else if(key == LV_KEY_RIGHT) next = false;
    }
    if(next != checked) {
        segmented_switch_set_checked(obj, next, true);
        lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
    }
}

static const lv_obj_class_t control_class = {
    .base_class = &lv_obj_class,
    .instance_size = sizeof(segmented_switch_t),
    .event_cb = control_event,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .name = "segmented_switch",
};

lv_obj_t * segmented_switch_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&control_class, parent);
    lv_obj_class_init_obj(obj);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 320, 64);
    lv_obj_set_scrollable(obj, false);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x302D29), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x655F55), 0);
    lv_obj_set_style_border_opa(obj, 150, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xFFF7EE), 0);
    lv_obj_set_style_opa(obj, LV_OPA_40, LV_STATE_DISABLED);
    lv_obj_set_style_outline_width(obj, 2, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_color(obj, lv_color_hex(0xB8AB97), LV_STATE_FOCUS_KEY);
    segmented_switch_t * control = (segmented_switch_t *)obj;
    control->thumb = lv_obj_create(obj);
    lv_obj_remove_style_all(control->thumb);
    lv_obj_set_clickable(control->thumb, false);
    lv_obj_set_scrollable(control->thumb, false);
    lv_obj_set_style_radius(control->thumb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(control->thumb, lv_color_hex(0xA69D8E), 0);
    lv_obj_set_style_bg_grad_color(control->thumb, lv_color_hex(0x7D7568), 0);
    lv_obj_set_style_bg_grad_dir(control->thumb, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(control->thumb, LV_OPA_COVER, 0);
    control->left = lv_label_create(obj);
    control->right = lv_label_create(obj);
    lv_obj_set_clickable(control->left, false);
    lv_obj_set_clickable(control->right, false);
    lv_label_set_long_mode(control->left, LV_LABEL_LONG_DOT);
    lv_label_set_long_mode(control->right, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(control->left, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(control->right, LV_TEXT_ALIGN_CENTER, 0);
    segmented_switch_set_labels(obj, "ON", "OFF");
    segmented_switch_set_checked(obj, true, false);
    return obj;
}

void segmented_switch_set_labels(lv_obj_t * obj, const char * left, const char * right)
{
    segmented_switch_t * control = (segmented_switch_t *)obj;
    lv_label_set_text(control->left, left);
    lv_label_set_text(control->right, right);
}
