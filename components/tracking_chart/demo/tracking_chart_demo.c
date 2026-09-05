#include "tracking_chart/demo/tracking_chart_demo.h"

#include <math.h>
#include "tracking_chart/tracking_chart.h"
#include "radial_background/radial_background.h"
#include "glass_button/glass_button.h"

#define DEMO_DURATION_MS 120000
#define DEMO_SAMPLE_MS 100
#define DEMO_TARGET_SAMPLE_MS 250
#define DEMO_SINE_PERIOD_MS 10000

typedef struct {
    lv_obj_t * chart;
    lv_obj_t * stop;
    lv_obj_t * icon;
    lv_obj_t * background;
    uint32_t paused_tick;
    int32_t baseline_y;
    bool paused;
    lv_timer_t * timer;
    uint32_t start_tick;
    uint32_t next_sample;
} tracking_demo_t;

static float target_value(uint32_t time_ms)
{
    float ramp = fminf(1.0f, time_ms / 6000.0f);
    ramp = ramp * ramp * (3.0f - 2.0f * ramp);
    return ramp * (24.0f + 16.0f * sinf(6.28318530718f * time_ms / DEMO_SINE_PERIOD_MS));
}

static float actual_value(uint32_t time_ms)
{
    uint32_t time = time_ms > 2000 ? time_ms - 2000 : 0;
    float value = target_value(time);
    return fmaxf(0, value * 0.94f + 0.8f * sinf(time_ms * 0.0005f) * fminf(1.0f, time / 6000.0f));
}

static void update_background(tracking_demo_t * demo)
{
    lv_point_t point;
    ui_tracking_chart_get_actual_point(demo->chart, &point);
    ui_radial_background_set_center(demo->background,
                                   lv_obj_get_x(demo->chart) + point.x,
                                   lv_obj_get_y(demo->chart) + point.y);
    float rise = LV_CLAMP(0.0f, (demo->baseline_y - point.y) / 140.0f, 1.0f);
    uint8_t mix = (uint8_t)(rise * rise * 255);
    const ui_radial_background_stop_t colors[] = {
        {lv_color_mix(lv_color_hex(0xAD4902), lv_color_hex(0x341008), mix), 20},
        {lv_color_mix(lv_color_hex(0x510900), lv_color_hex(0x200C08), mix), 150},
        {lv_color_hex(0x141414), 330},
    };
    if(ui_radial_background_set_stops(demo->background, colors, LV_ARRAYLEN(colors)) != LV_RESULT_OK)
        LV_LOG_ERROR("Cannot update demo background");
}

static void stop_sampling(tracking_demo_t * demo)
{
    lv_timer_pause(demo->timer);
    lv_obj_set_state(demo->stop, LV_STATE_DISABLED, true);
}

static void sample_timer(lv_timer_t * timer)
{
    tracking_demo_t * demo = lv_timer_get_user_data(timer);
    uint32_t elapsed = LV_MIN(lv_tick_elaps(demo->start_tick), DEMO_DURATION_MS);
    while(demo->next_sample <= elapsed) {
        if(ui_tracking_chart_append_actual(demo->chart, demo->next_sample,
                                            actual_value(demo->next_sample)) != LV_RESULT_OK) {
            LV_LOG_ERROR("Cannot append tracking demo sample");
            stop_sampling(demo);
            return;
        }
        demo->next_sample += DEMO_SAMPLE_MS;
    }
    update_background(demo);
    if(elapsed == DEMO_DURATION_MS) stop_sampling(demo);
}

static void stop_clicked(lv_event_t * e)
{
    tracking_demo_t * demo = lv_event_get_user_data(e);
    if(demo->paused) {
        demo->start_tick += lv_tick_elaps(demo->paused_tick);
        demo->paused = false;
        lv_label_set_text(demo->icon, LV_SYMBOL_PAUSE);
        lv_timer_resume(demo->timer);
    }
    else {
        demo->paused_tick = lv_tick_get();
        demo->paused = true;
        lv_timer_pause(demo->timer);
        lv_label_set_text(demo->icon, LV_SYMBOL_PLAY);
    }
}

static void demo_deleted(lv_event_t * e)
{
    tracking_demo_t * demo = lv_event_get_user_data(e);
    if(demo->timer) lv_timer_delete(demo->timer);
    lv_free(demo);
}

void tracking_chart_demo(void)
{
    tracking_demo_t * demo = lv_malloc_zeroed(sizeof(*demo));
    if(demo == NULL) { LV_LOG_ERROR("Cannot allocate tracking demo"); return; }
    lv_obj_t * screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x202020), 0);
    lv_obj_t * face = lv_obj_create(screen);
    lv_obj_remove_style_all(face);
    lv_obj_set_size(face, 600, 600);
    lv_obj_center(face);
    lv_obj_set_scrollable(face, false);
    lv_obj_set_clickable(face, false);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(face, true, 0);
    lv_obj_set_style_bg_color(face, lv_color_hex(0x141414), 0);
    lv_obj_set_style_bg_opa(face, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(face, demo_deleted, LV_EVENT_DELETE, demo);

    demo->background = ui_radial_background_create(face);
    demo->chart = ui_tracking_chart_create(face);
    lv_obj_set_pos(demo->chart, 40, 165);
    ui_tracking_chart_set_fade_width(demo->chart, 48);
    ui_tracking_chart_sample_t target[DEMO_DURATION_MS / DEMO_TARGET_SAMPLE_MS + 1];
    for(uint32_t i = 0; i < LV_ARRAYLEN(target); i++) {
        uint32_t time = i * DEMO_TARGET_SAMPLE_MS;
        target[i] = (ui_tracking_chart_sample_t){time, target_value(time)};
    }
    if(ui_tracking_chart_set_window_ms(demo->chart, 20000) != LV_RESULT_OK ||
       ui_tracking_chart_set_target(demo->chart, target, LV_ARRAYLEN(target)) != LV_RESULT_OK ||
       ui_tracking_chart_set_format(demo->chart, "ml/s", 1) != LV_RESULT_OK) {
        LV_LOG_ERROR("Cannot configure tracking demo");
        lv_obj_delete(face);
        return;
    }

    lv_obj_t * title = lv_label_create(face);
    lv_label_set_text(title, "Double Espresso");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFF7EE), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_36, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 80);

    demo->stop = ui_glass_button_create(face);
    lv_obj_set_size(demo->stop, 76, 76);
    lv_obj_align(demo->stop, LV_ALIGN_BOTTOM_MID, 0, -52);
    demo->icon = lv_label_create(demo->stop);
    lv_label_set_text(demo->icon, LV_SYMBOL_PAUSE);
    lv_obj_set_style_text_font(demo->icon, &lv_font_montserrat_24, 0);
    lv_obj_set_clickable(demo->icon, false);
    lv_obj_center(demo->icon);
    lv_obj_add_event_cb(demo->stop, stop_clicked, LV_EVENT_CLICKED, demo);

    lv_obj_update_layout(face);
    lv_point_t initial_point;
    ui_tracking_chart_get_actual_point(demo->chart, &initial_point);
    demo->baseline_y = initial_point.y;
    update_background(demo);
    demo->start_tick = lv_tick_get();
    demo->next_sample = 0;
    demo->timer = lv_timer_create(sample_timer, DEMO_SAMPLE_MS, demo);
    if(demo->timer == NULL) {
        LV_LOG_ERROR("Cannot create tracking demo timer");
        lv_obj_delete(face);
    }
}
