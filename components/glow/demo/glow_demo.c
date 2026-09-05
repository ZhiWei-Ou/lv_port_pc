#include "glow/demo/glow_demo.h"

#include "glow/glow.h"
#include "lvgl/lvgl.h"

#define GLOW_GRID_MARGIN 12
#define GLOW_GRID_GAP    8
#define GLOW_CARD_SIZE   226

typedef struct {
    const char * title;
    int16_t center_x;
    int16_t center_y;
    uint16_t eccentricity;
    int16_t angle;
    uint16_t spread;
    uint8_t gradient_variant;
    bool breathing;
    bool fill_from_below;
    bool fills_card;
    bool moves_in_frame;
} glow_example_t;

static void glow_breath_anim_cb(void * var, int32_t value)
{
    glow_set_spread((lv_obj_t *)var, (uint16_t)value);
}

static void glow_move_y_anim_cb(void * var, int32_t value)
{
    lv_obj_set_y((lv_obj_t *)var, value);
}

static void create_glow_example(lv_obj_t * parent, const glow_example_t * example, int32_t x, int32_t y)
{
    const glow_gradient_stop_t amber_stops[] = {
        {lv_color_hex(0xFFE0C2), LV_OPA_90, 0},
        {lv_color_hex(0xD45900), LV_OPA_70, 70},
        {lv_color_hex(0x9A3900), LV_OPA_30, 160},
        {lv_color_hex(0x4A1900), LV_OPA_TRANSP, 255},
    };
    const glow_gradient_stop_t ember_stops[] = {
        {lv_color_hex(0xFFD0A3), LV_OPA_80, 0},
        {lv_color_hex(0xD45900), LV_OPA_70, 80},
        {lv_color_hex(0x8B2A00), LV_OPA_30, 170},
        {lv_color_hex(0x431400), LV_OPA_TRANSP, 255},
    };
    const glow_gradient_stop_t * stops = example->gradient_variant ? ember_stops : amber_stops;

    lv_obj_t * card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, GLOW_CARD_SIZE, GLOW_CARD_SIZE);
    lv_obj_set_scrollable(card, false);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x273449), 0);
    lv_obj_set_overflow_visible(card, false);

    lv_obj_t * glow_parent = card;
    if(example->moves_in_frame) {
        glow_parent = lv_obj_create(card);
        lv_obj_remove_style_all(glow_parent);
        lv_obj_set_pos(glow_parent, 14, 48);
        lv_obj_set_size(glow_parent, GLOW_CARD_SIZE - 28, GLOW_CARD_SIZE - 62);
        lv_obj_set_scrollable(glow_parent, false);
        lv_obj_set_style_bg_opa(glow_parent, LV_OPA_TRANSP, 0);
        lv_obj_set_style_radius(glow_parent, 8, 0);
        lv_obj_set_style_border_width(glow_parent, 1, 0);
        lv_obj_set_style_border_color(glow_parent, lv_color_hex(0x273449), 0);
        lv_obj_set_overflow_visible(glow_parent, false);
    }

    lv_obj_t * glow = glow_create(glow_parent);
    if(example->fill_from_below || example->fills_card) {
        lv_obj_set_size(glow, GLOW_CARD_SIZE * 3, GLOW_CARD_SIZE * 3);
        lv_obj_set_pos(glow, -GLOW_CARD_SIZE, -GLOW_CARD_SIZE);
    }
    glow_set_gradient(glow, stops, 4);
    if(example->fill_from_below) {
        glow_set_center(glow, 0, GLOW_CARD_SIZE / 2 + 56);
    }
    else {
        glow_set_center(glow, example->center_x, example->center_y);
    }
    glow_set_eccentricity(glow, example->eccentricity);
    glow_set_angle(glow, example->angle);
    if(example->fill_from_below) {
        glow_set_spread(glow, 0);
        lv_anim_t fill;
        lv_anim_init(&fill);
        lv_anim_set_var(&fill, glow);
        lv_anim_set_values(&fill, 0, example->spread);
        lv_anim_set_duration(&fill, 1800);
        lv_anim_set_reverse_duration(&fill, 1800);
        lv_anim_set_repeat_count(&fill, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&fill, glow_breath_anim_cb);
        lv_anim_set_path_cb(&fill, lv_anim_path_ease_in_out);
        lv_anim_start(&fill);
    }
    else {
        glow_set_spread(glow, example->spread);
    }

    if(example->moves_in_frame) {
        lv_anim_t move;
        lv_anim_init(&move);
        lv_anim_set_var(&move, glow);
        lv_anim_set_values(&move, -90, 70);
        lv_anim_set_duration(&move, 2200);
        lv_anim_set_reverse_duration(&move, 2200);
        lv_anim_set_repeat_count(&move, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&move, glow_move_y_anim_cb);
        lv_anim_set_path_cb(&move, lv_anim_path_ease_in_out);
        lv_anim_start(&move);
    }

    lv_obj_t * label = lv_label_create(card);
    lv_label_set_text(label, example->title);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFF7ED), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_90, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 12, 10);

    if(!example->breathing) {
        return;
    }

    lv_anim_t breath;
    lv_anim_init(&breath);
    lv_anim_set_var(&breath, glow);
    lv_anim_set_values(&breath, 480, example->spread);
    lv_anim_set_duration(&breath, 2200);
    lv_anim_set_reverse_duration(&breath, 2200);
    lv_anim_set_repeat_count(&breath, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&breath, glow_breath_anim_cb);
    lv_anim_set_path_cb(&breath, lv_anim_path_ease_in_out);
    lv_anim_start(&breath);
}

void glow_demo(void)
{
    static const glow_example_t examples[] = {
        {"Center",      0,    0,   0,   0,  720, 0, false, false, false, false},
        {"Below fill",  0,    0,   0,   0, 1000, 0, false, true,  false, false},
        {"Top left",  -78,  -72,   0,   0,  740, 1, false, false, false, false},
        {"Top right",  78,  -72, 420, -35,  760, 0, false, false, false, false},
        {"Wide",        0,   10, 650,   0,  790, 1, false, false, false, false},
        {"Diagonal",  -34,   18, 560,  38,  760, 0, false, false, false, false},
        {"Corner",    -82,   82, 350, -55,  820, 1, false, false, false, false},
        {"Move frame",  0,    0,   0,  18,  900, 0, false, false, false, true },
        {"Breathe",     0,   18, 500, -22, 1000, 1, true,  false, true,  false},
    };
    lv_obj_t * screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    for(uint8_t i = 0; i < LV_ARRAYLEN(examples); i++) {
        int32_t column = i % 3;
        int32_t row = i / 3;
        create_glow_example(screen, &examples[i],
                            GLOW_GRID_MARGIN + column * (GLOW_CARD_SIZE + GLOW_GRID_GAP),
                            GLOW_GRID_MARGIN + row * (GLOW_CARD_SIZE + GLOW_GRID_GAP));
    }
}
