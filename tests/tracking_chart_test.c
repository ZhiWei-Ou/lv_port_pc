#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "tracking_chart/tracking_chart.h"
#include "tracking_chart/demo/tracking_chart_demo.h"
#include "src/hal/hal.h"

#define CHECK(condition) do { if(!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); \
} } while(0)

static uint8_t pixels[700 * 700 * 4];
static const char * snapshot_dir;

typedef struct {
    unsigned target_paths;
    unsigned actual_paths;
    float last_x;
    float last_y;
    float min_x;
    lv_opa_t min_opa;
    float min_y;
    float max_y;
} path_observation_t;

static void flush(lv_display_t * display, const lv_area_t * area, uint8_t * data)
{
    (void)area; (void)data;
    lv_display_flush_ready(display);
}

static lv_obj_t * find_label(lv_obj_t * obj, const char * text)
{
    if(lv_obj_has_class(obj, &lv_label_class) && strcmp(lv_label_get_text(obj), text) == 0) return obj;
    for(uint32_t i = 0; i < lv_obj_get_child_count(obj); i++) {
        lv_obj_t * found = find_label(lv_obj_get_child(obj, i), text);
        if(found) return found;
    }
    return NULL;
}

static lv_obj_t * find_button(lv_obj_t * obj)
{
    if(lv_obj_has_class(obj, &lv_button_class)) return obj;
    for(uint32_t i = 0; i < lv_obj_get_child_count(obj); i++) {
        lv_obj_t * found = find_button(lv_obj_get_child(obj, i));
        if(found) return found;
    }
    return NULL;
}

static void observe_curve(lv_event_t * e)
{
    lv_draw_line_dsc_t * line = lv_draw_task_get_line_dsc(lv_event_get_draw_task(e));
    if(line == NULL || line->points == NULL) return;
    path_observation_t * observation = lv_event_get_user_data(e);
    bool actual = line->color.green > 200;
    if(actual) observation->actual_paths++;
    else observation->target_paths++;
    for(int32_t i = 0; i < line->point_cnt; i++) {
        lv_point_precise_t point = line->points[i];
        if(point.x == LV_DRAW_LINE_POINT_NONE) continue;
        /* Chart at (90,160), 520x270. Curves must remain left of center. */
        CHECK(point.x >= 97.99f && point.x <= 350.01f);
        CHECK(point.y >= 175.99f && point.y <= 354.01f);
        if(actual) {
            observation->min_opa = LV_MIN(observation->min_opa, line->opa);
            observation->min_x = fminf(observation->min_x, point.x);
            observation->last_x = point.x;
            observation->last_y = point.y;
            observation->min_y = fminf(observation->min_y, point.y);
            observation->max_y = fmaxf(observation->max_y, point.y);
        }
    }
}

static void render(lv_obj_t * chart, path_observation_t * observation)
{
    *observation = (path_observation_t){.min_x = INFINITY, .min_y = INFINITY,
                                      .max_y = -INFINITY, .min_opa = LV_OPA_COVER};
    lv_obj_invalidate(chart);
    lv_refr_now(NULL);
}

static void snapshot(const char * name)
{
    if(snapshot_dir == NULL) return;
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.ppm", snapshot_dir, name);
    FILE * file = fopen(path, "wb");
    CHECK(file != NULL);
    fprintf(file, "P6\n700 700\n255\n");
    for(size_t i = 0; i < 700 * 700; i++) {
        uint8_t rgb[] = {pixels[i * 4 + 2], pixels[i * 4 + 1], pixels[i * 4]};
        CHECK(fwrite(rgb, 1, 3, file) == 3);
    }
    CHECK(fclose(file) == 0);
}

static void test_chart(void)
{
    lv_obj_t * chart = tracking_chart_create(lv_screen_active());
    lv_obj_set_pos(chart, 90, 160);
    CHECK(tracking_chart_append_actual(chart, 0, 0) == LV_RESULT_INVALID);
    CHECK(tracking_chart_set_format(chart, "ml/s", 1) == LV_RESULT_OK);
    CHECK(tracking_chart_set_format(chart, NULL, 1) == LV_RESULT_INVALID);
    CHECK(tracking_chart_set_format(chart, "bad", 7) == LV_RESULT_INVALID);
    tracking_chart_sample_t target[] = {{0, 0}, {12000, 40}, {120000, 20}};
    CHECK(tracking_chart_set_target(chart, target, 3) == LV_RESULT_OK);
    CHECK(find_label(chart, "--") && find_label(chart, "ml/s"));
    target[1].value = 999; /* The chart owns a copy. */

    path_observation_t observation;
    lv_obj_set_send_draw_task_events(chart, true);
    lv_obj_add_event_cb(chart, observe_curve, LV_EVENT_DRAW_TASK_ADDED, &observation);
    render(chart, &observation);
    CHECK(observation.actual_paths == 0 && observation.target_paths == 0);
    CHECK(find_label(chart, "--") && find_label(chart, "0"));
    snapshot("chart_empty");
    CHECK(tracking_chart_append_actual(chart, 0, 2) == LV_RESULT_OK);
    render(chart, &observation);
    CHECK(observation.actual_paths == 0 && observation.target_paths == 0);
    CHECK(find_label(chart, "002.0"));
    CHECK(tracking_chart_append_actual(chart, 250, 4) == LV_RESULT_OK);
    CHECK(tracking_chart_append_actual(chart, 1250, 15) == LV_RESULT_OK);
    CHECK(tracking_chart_append_actual(chart, 12000, 35) == LV_RESULT_OK);
    render(chart, &observation);
    CHECK(observation.actual_paths == 1 && observation.target_paths == 1);
    /* At 12 seconds, only 12/20 of the fixed-width history is filled. */
    CHECK(fabsf(observation.min_x - 198.8f) < 0.01f);
    CHECK(fabsf(observation.last_x - 350) < 0.01f);
    CHECK(fabsf(observation.last_y - (354 - 35.0f / 44 * 178)) < 0.01f);
    CHECK(find_label(chart, "035.0") && find_label(chart, "12"));
    snapshot("chart_12");

    /* Runtime display settings change the existing trace without resetting it. */
    CHECK(tracking_chart_set_window_ms(chart, 40000) == LV_RESULT_OK);
    CHECK(tracking_chart_set_window_ms(chart, 0) == LV_RESULT_INVALID);
    render(chart, &observation);
    CHECK(fabsf(observation.min_x - 274.4f) < 0.01f);
    CHECK(find_label(chart, "035.0") && find_label(chart, "12"));
    tracking_chart_set_fade_width(chart, 252);
    render(chart, &observation);
    CHECK(observation.min_opa < LV_OPA_COVER);
    tracking_chart_set_fade_width(chart, 0);
    render(chart, &observation);
    CHECK(observation.min_opa == LV_OPA_COVER);
    CHECK(tracking_chart_set_window_ms(chart, 20000) == LV_RESULT_OK);
    tracking_chart_set_fade_width(chart, 48);

    CHECK(tracking_chart_append_actual(chart, 12000, 1) == LV_RESULT_INVALID);
    CHECK(tracking_chart_append_actual(chart, 1000, 1) == LV_RESULT_INVALID);
    CHECK(tracking_chart_append_actual(chart, 120001, 1) == LV_RESULT_INVALID);
    CHECK(tracking_chart_append_actual(chart, 13000, NAN) == LV_RESULT_INVALID);
    CHECK(tracking_chart_append_actual(chart, 13000, INFINITY) == LV_RESULT_INVALID);
    CHECK(tracking_chart_set_target(chart, NULL, 3) == LV_RESULT_INVALID);
    tracking_chart_sample_t invalid[] = {{0, 0}, {0, 1}};
    CHECK(tracking_chart_set_target(chart, invalid, 2) == LV_RESULT_INVALID);
    CHECK(find_label(chart, "035.0"));

    CHECK(tracking_chart_append_actual(chart, 60000, -100) == LV_RESULT_OK);
    render(chart, &observation);
    CHECK(find_label(chart, "-100.0") && find_label(chart, "60"));
    snapshot("chart_below_range");
    CHECK(tracking_chart_append_actual(chart, 120000, 100) == LV_RESULT_OK);
    render(chart, &observation);
    CHECK(find_label(chart, "100.0") && find_label(chart, "120"));
    snapshot("chart_above_range");

    tracking_chart_reset(chart);
    CHECK(find_label(chart, "--") && find_label(chart, "ml/s"));
    const tracking_chart_sample_t shape[] = {{0, 0}, {1000, 40}, {1200, 40}, {90000, 5}, {120000, 20}};
    for(size_t i = 0; i < LV_ARRAYLEN(shape); i++) {
        CHECK(tracking_chart_append_actual(chart, shape[i].time_ms, shape[i].value) == LV_RESULT_OK);
    }
    render(chart, &observation);
    /* Only 100..120 seconds remain: the old peak of 40 is off screen,
     * and the sparse 90..120 second segment is clipped at the left edge. */
    CHECK(fabsf(observation.min_x - 98) < 0.01f);
    CHECK(observation.min_y >= 354 - 20.0f / 44 * 178 - 0.01f);
    CHECK(observation.max_y <= 354 + 0.01f);
    snapshot("chart_120");

    const tracking_chart_sample_t zero[] = {{0, 0}, {120000, 0}};
    CHECK(tracking_chart_set_target(chart, zero, 2) == LV_RESULT_OK);
    CHECK(find_label(chart, "--"));
    CHECK(tracking_chart_append_actual(chart, 1000, 0) == LV_RESULT_OK);
    render(chart, &observation);
    CHECK(find_label(chart, "000.0"));
    lv_obj_delete(chart);
}

static void test_demo(void)
{
    tracking_chart_demo();
    lv_obj_t * screen = lv_screen_active();
    CHECK(find_label(screen, "Double Espresso"));
    lv_refr_now(NULL);
    snapshot("demo_initial");
    lv_tick_inc(12000); lv_timer_handler(); lv_refr_now(NULL);
    CHECK(find_label(screen, "12"));
    snapshot("demo_12");
    lv_obj_send_event(find_button(screen), LV_EVENT_CLICKED, NULL);
    lv_tick_inc(5000); lv_timer_handler();
    CHECK(find_label(screen, "12"));
    lv_obj_send_event(find_button(screen), LV_EVENT_CLICKED, NULL);
    lv_tick_inc(48000); lv_timer_handler(); lv_refr_now(NULL);
    CHECK(find_label(screen, "60"));
    snapshot("demo_60");
    lv_tick_inc(60000); lv_timer_handler(); lv_refr_now(NULL);
    CHECK(find_label(screen, "120"));
    CHECK(lv_obj_has_state(find_button(screen), LV_STATE_DISABLED));
    snapshot("demo_120");
    lv_tick_inc(5000); lv_timer_handler();
    CHECK(find_label(screen, "120"));
    lv_obj_delete(lv_obj_get_parent(find_label(screen, "Double Espresso")));
    tracking_chart_demo();
    lv_obj_delete(lv_obj_get_parent(find_label(screen, "Double Espresso")));
    lv_tick_inc(1000); lv_timer_handler(); /* Deleted demo must not retain its sampling timer. */
}

static void sdl_step(uint32_t duration)
{
    uint32_t start = SDL_GetTicks();
    do { lv_timer_handler(); SDL_Delay(2); } while(SDL_GetTicks() - start < duration);
}

static void sdl_click(lv_display_t * display, lv_obj_t * button, uint32_t type)
{
    lv_area_t area;
    lv_obj_get_coords(button, &area);
    SDL_Event event = {0};
    event.type = type;
    event.button.windowID = SDL_GetWindowID(lv_sdl_window_get_window(display));
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = (area.x1 + area.x2) / 2;
    event.button.y = (area.y1 + area.y2) / 2;
    CHECK(SDL_PushEvent(&event) == 1);
}

static void test_sdl_stop(void)
{
    lv_display_t * display = sdl_hal_init(700, 700);
    CHECK(display != NULL);
    tracking_chart_demo();
    sdl_step(1200);
    lv_obj_t * button = find_button(lv_screen_active());
    CHECK(button != NULL && find_label(lv_screen_active(), "1"));
    sdl_click(display, button, SDL_MOUSEBUTTONDOWN); sdl_step(170);
    CHECK(lv_obj_get_style_transform_scale_x(button, 0) == 243);
    sdl_click(display, button, SDL_MOUSEBUTTONUP); sdl_step(250);
    CHECK(!lv_obj_has_state(button, LV_STATE_DISABLED));
    CHECK(lv_obj_get_style_transform_scale_x(button, 0) == 256);
    sdl_step(1100);
    CHECK(find_label(lv_screen_active(), "1"));
    sdl_click(display, button, SDL_MOUSEBUTTONDOWN); sdl_step(170);
    sdl_click(display, button, SDL_MOUSEBUTTONUP); sdl_step(1100);
    CHECK(find_label(lv_screen_active(), "2") || find_label(lv_screen_active(), "3"));
    lv_obj_delete(lv_obj_get_parent(find_label(lv_screen_active(), "Double Espresso")));
    sdl_step(150);
    lv_sdl_quit();
}

int main(int argc, char ** argv)
{
    lv_init();
    if(argc > 1 && strcmp(argv[1], "--sdl") == 0) test_sdl_stop();
    else {
        if(argc > 1) snapshot_dir = argv[1];
        lv_display_t * display = lv_display_create(700, 700);
        lv_display_set_buffers(display, pixels, NULL, sizeof(pixels), LV_DISPLAY_RENDER_MODE_FULL);
        lv_display_set_flush_cb(display, flush);
        test_chart();
        test_demo();
        lv_display_delete(display);
    }
    lv_deinit();
    puts("Tracking chart integration checks passed");
    return 0;
}
