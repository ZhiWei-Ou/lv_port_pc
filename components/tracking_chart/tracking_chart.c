#include "tracking_chart/tracking_chart.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "lvgl/src/core/lv_obj_class_private.h"
#include "lvgl/src/core/lv_obj_private.h"

#define CHART_WINDOW_MS 20000

typedef struct {
    ui_tracking_chart_sample_t sample;
    double slope;
} chart_point_t;

typedef struct {
    lv_obj_t obj;
    chart_point_t * target;
    chart_point_t * actual;
    uint32_t target_count;
    uint32_t actual_count;
    uint32_t actual_capacity;
    double y_min;
    double y_max;
    uint8_t decimals;
    lv_obj_t * value_label;
    lv_obj_t * unit_label;
    lv_obj_t * time_label;
    lv_obj_t * seconds_label;
} tracking_chart_t;

typedef struct {
    int32_t left;
    int32_t center;
    int32_t right;
    int32_t top;
    int32_t bottom;
    int32_t axis;
} chart_geometry_t;

static void chart_event(const lv_obj_class_t * class_p, lv_event_t * e);
static void chart_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void refresh_labels(tracking_chart_t * chart);

static const lv_obj_class_t tracking_chart_class = {
    .event_cb = chart_event,
    .destructor_cb = chart_destructor,
    .width_def = 520,
    .height_def = 270,
    .instance_size = sizeof(tracking_chart_t),
    .base_class = &lv_obj_class,
    .name = "ui_tracking_chart",
};

/* PCHIP slopes: harmonic interior tangents and limited endpoint tangents.
 * The interpolation passes through samples without introducing new extrema. */
static double point_slope(const chart_point_t * points, uint32_t count, uint32_t i)
{
    if(count < 2) return 0;
    if(count == 2) {
        return ((double)points[1].sample.value - points[0].sample.value) /
               (points[1].sample.time_ms - points[0].sample.time_ms);
    }
    bool endpoint = i == 0 || i == count - 1;
    uint32_t a = i == 0 ? 0 : (i == count - 1 ? count - 3 : i - 1);
    double h0 = points[a + 1].sample.time_ms - points[a].sample.time_ms;
    double h1 = points[a + 2].sample.time_ms - points[a + 1].sample.time_ms;
    double d0 = ((double)points[a + 1].sample.value - points[a].sample.value) / h0;
    double d1 = ((double)points[a + 2].sample.value - points[a + 1].sample.value) / h1;
    if(!endpoint) {
        if(d0 * d1 <= 0) return 0;
        double w0 = 2 * h1 + h0;
        double w1 = h1 + 2 * h0;
        return (w0 + w1) / (w0 / d0 + w1 / d1);
    }
    if(i == count - 1) {
        double swap = h0; h0 = h1; h1 = swap;
        swap = d0; d0 = d1; d1 = swap;
    }
    double slope = ((2 * h0 + h1) * d0 - h0 * d1) / (h0 + h1);
    if(slope * d0 <= 0) return 0;
    if(d0 * d1 <= 0 && fabs(slope) > fabs(3 * d0)) return 3 * d0;
    return slope;
}

static double interpolate(const chart_point_t * a, const chart_point_t * b, double time)
{
    double h = b->sample.time_ms - a->sample.time_ms;
    double t = (time - a->sample.time_ms) / h;
    double t2 = t * t;
    double t3 = t2 * t;
    return (2 * t3 - 3 * t2 + 1) * a->sample.value +
           (t3 - 2 * t2 + t) * h * a->slope +
           (-2 * t3 + 3 * t2) * b->sample.value + (t3 - t2) * h * b->slope;
}

static chart_geometry_t geometry(lv_obj_t * obj)
{
    int32_t width = lv_obj_get_width(obj);
    int32_t height = lv_obj_get_height(obj);
    chart_geometry_t g = {8, width / 2, width - 8, 16, height - 80, height - 64};
    return g;
}

static double value_y(const tracking_chart_t * chart, const chart_geometry_t * g, double value)
{
    return g->bottom - (value - chart->y_min) / (chart->y_max - chart->y_min) * (g->bottom - g->top);
}

lv_obj_t * ui_tracking_chart_create(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&tracking_chart_class, parent);
    lv_obj_class_init_obj(obj);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 520, 270);
    lv_obj_set_clickable(obj, false);
    lv_obj_set_scrollable(obj, false);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xFFF7EE), 0);
    tracking_chart_t * chart = (tracking_chart_t *)obj;
    chart->y_max = 1;
    chart->decimals = 1;
    chart->value_label = lv_label_create(obj);
    chart->unit_label = lv_label_create(obj);
    chart->time_label = lv_label_create(obj);
    chart->seconds_label = lv_label_create(obj);
    lv_obj_set_style_text_font(chart->value_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_font(chart->time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_font(chart->unit_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_font(chart->seconds_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(chart->unit_label, "");
    lv_label_set_text(chart->seconds_label, "sec");
    refresh_labels(chart);
    return obj;
}

lv_result_t ui_tracking_chart_set_target(lv_obj_t * obj,
                                         const ui_tracking_chart_sample_t * samples,
                                         uint16_t count)
{
    if(samples == NULL || count < 2 || samples[0].time_ms != 0) return LV_RESULT_INVALID;
    double low = 0;
    double high = 0;
    for(uint32_t i = 0; i < count; i++) {
        if(!isfinite(samples[i].value) || (i && samples[i].time_ms <= samples[i - 1].time_ms)) {
            return LV_RESULT_INVALID;
        }
        low = fmin(low, samples[i].value);
        high = fmax(high, samples[i].value);
    }
    chart_point_t * target = lv_malloc(sizeof(*target) * count);
    if(target == NULL) return LV_RESULT_INVALID;
    for(uint32_t i = 0; i < count; i++) target[i].sample = samples[i];
    for(uint32_t i = 0; i < count; i++) target[i].slope = point_slope(target, count, i);

    tracking_chart_t * chart = (tracking_chart_t *)obj;
    lv_free(chart->target);
    chart->target = target;
    chart->target_count = count;
    double padding = (high - low) * 0.1;
    chart->y_min = low - padding;
    chart->y_max = high == low ? 1 : high + padding;
    ui_tracking_chart_reset(obj);
    return LV_RESULT_OK;
}

lv_result_t ui_tracking_chart_append_actual(lv_obj_t * obj, uint32_t time_ms, float value)
{
    tracking_chart_t * chart = (tracking_chart_t *)obj;
    uint32_t count = chart->actual_count;
    if(!chart->target_count || !isfinite(value) ||
       time_ms > chart->target[chart->target_count - 1].sample.time_ms ||
       (count && time_ms <= chart->actual[count - 1].sample.time_ms)) {
        return LV_RESULT_INVALID;
    }
    if(count == chart->actual_capacity) {
        uint32_t capacity = count ? count * 2 : 128;
        if(capacity < count || SIZE_MAX / capacity < sizeof(*chart->actual)) return LV_RESULT_INVALID;
        chart_point_t * actual = lv_realloc(chart->actual, capacity * sizeof(*actual));
        if(actual == NULL) return LV_RESULT_INVALID;
        chart->actual = actual;
        chart->actual_capacity = capacity;
    }
    chart->actual[count].sample = (ui_tracking_chart_sample_t){time_ms, value};
    chart->actual_count = ++count;
    chart->actual[count - 1].slope = point_slope(chart->actual, count, count - 1);
    if(count >= 2) chart->actual[count - 2].slope = point_slope(chart->actual, count, count - 2);
    if(count == 3) chart->actual[0].slope = point_slope(chart->actual, count, 0);
    /* Keep two points before the window for interpolation and tangent updates. */
    uint32_t start = time_ms > CHART_WINDOW_MS ? time_ms - CHART_WINDOW_MS : 0;
    uint32_t discard = 0;
    while(discard + 2 < count && chart->actual[discard + 2].sample.time_ms < start) discard++;
    if(discard) {
        chart->actual_count -= discard;
        memmove(chart->actual, chart->actual + discard, chart->actual_count * sizeof(*chart->actual));
    }
    refresh_labels(chart);
    lv_obj_invalidate(obj);
    return LV_RESULT_OK;
}

lv_result_t ui_tracking_chart_set_format(lv_obj_t * obj, const char * unit, uint8_t decimals)
{
    if(unit == NULL || decimals > 6) return LV_RESULT_INVALID;
    tracking_chart_t * chart = (tracking_chart_t *)obj;
    chart->decimals = decimals;
    lv_label_set_text(chart->unit_label, unit);
    refresh_labels(chart);
    return LV_RESULT_OK;
}

void ui_tracking_chart_reset(lv_obj_t * obj)
{
    tracking_chart_t * chart = (tracking_chart_t *)obj;
    lv_free(chart->actual);
    chart->actual = NULL;
    chart->actual_count = 0;
    chart->actual_capacity = 0;
    refresh_labels(chart);
    lv_obj_invalidate(obj);
}

static void refresh_labels(tracking_chart_t * chart)
{
    if(chart->seconds_label == NULL) return; /* Size events during construction. */
    chart_geometry_t g = geometry(&chart->obj);
    double value = 0;
    uint32_t time = 0;
    if(chart->actual_count) {
        ui_tracking_chart_sample_t latest = chart->actual[chart->actual_count - 1].sample;
        value = latest.value;
        time = latest.time_ms;
        char text[64];
        snprintf(text, sizeof(text), "%05.*f", chart->decimals, value);
        lv_label_set_text(chart->value_label, text);
    }
    else lv_label_set_text(chart->value_label, "--");
    lv_label_set_text_fmt(chart->time_label, "%u", (unsigned)(time / 1000));
    int32_t y = (int32_t)value_y(chart, &g, LV_CLAMP(chart->y_min, value, chart->y_max));
    lv_obj_set_pos(chart->value_label, g.center + 18, LV_CLAMP(0, y - 26, LV_MAX(0, g.bottom - 48)));
    lv_obj_set_pos(chart->time_label, g.center + 18, g.axis + 16);
    lv_obj_align_to(chart->unit_label, chart->value_label, LV_ALIGN_OUT_RIGHT_TOP, 5, 8);
    lv_obj_align_to(chart->seconds_label, chart->time_label, LV_ALIGN_OUT_RIGHT_TOP, 5, 8);
}

static void draw_line(lv_layer_t * layer, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                      lv_color_t color, lv_opa_t opa)
{
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.p1 = (lv_point_precise_t){x0, y0};
    line.p2 = (lv_point_precise_t){x1, y1};
    line.color = color;
    line.opa = opa;
    line.width = 1;
    lv_draw_line(layer, &line);
}

/* Clip in value space before converting to pixel coordinates, including very
 * large finite inputs. Out-of-range runs are not flattened onto the plot edge. */
static uint32_t add_segment(lv_point_precise_t * path, uint32_t count, const tracking_chart_t * chart,
                            const chart_geometry_t * g, double x0, double v0, double x1, double v1)
{
    if((v0 < chart->y_min && v1 < chart->y_min) || (v0 > chart->y_max && v1 > chart->y_max)) return count;
    double from = 0;
    double to = 1;
    if(v0 != v1) {
        double a = (chart->y_min - v0) / (v1 - v0);
        double b = (chart->y_max - v0) / (v1 - v0);
        from = fmax(0, fmin(a, b));
        to = fmin(1, fmax(a, b));
    }
    path[count++] = (lv_point_precise_t){x0 + (x1 - x0) * from, value_y(chart, g, v0 + (v1 - v0) * from)};
    path[count++] = (lv_point_precise_t){x0 + (x1 - x0) * to, value_y(chart, g, v0 + (v1 - v0) * to)};
    path[count++] = (lv_point_precise_t){LV_DRAW_LINE_POINT_NONE, LV_DRAW_LINE_POINT_NONE};
    return count;
}

static void draw_curve(tracking_chart_t * chart, lv_layer_t * layer, const chart_geometry_t * g,
                       const chart_point_t * points, uint32_t count, uint32_t now, lv_color_t color, int32_t width)
{
    if(count < 2 || now == 0) return;
    size_t capacity = 3 * ((size_t)count + g->center - g->left + 2);
    lv_point_precise_t * path = lv_malloc(capacity * sizeof(*path));
    if(path == NULL) { LV_LOG_ERROR("Cannot allocate chart draw path"); return; }
    uint32_t used = 0;
    double scale = (double)(g->center - g->left) / CHART_WINDOW_MS;
    double window_start = (double)now - CHART_WINDOW_MS;
    for(uint32_t i = 0; i + 1 < count && points[i].sample.time_ms < now; i++) {
        if(points[i + 1].sample.time_ms <= window_start) continue;
        double start = fmax(points[i].sample.time_ms, window_start);
        double end = LV_MIN(points[i + 1].sample.time_ms, now);
        /* Give the software renderer a useful span for edge antialiasing.
         * One-pixel segments collapse to integer horizontal/vertical steps. */
        int32_t steps = LV_MAX(1, (int32_t)ceil((end - start) * scale / 8.0));
        double x0 = g->left + (start - window_start) * scale;
        double v0 = interpolate(&points[i], &points[i + 1], start);
        for(int32_t j = 1; j <= steps; j++) {
            double time = start + (end - start) * j / steps;
            double x1 = g->left + (time - window_start) * scale;
            double v1 = interpolate(&points[i], &points[i + 1], time);
            used = add_segment(path, used, chart, g, x0, v0, x1, v1);
            x0 = x1;
            v0 = v1;
        }
    }
    lv_draw_line_dsc_t line;
    lv_draw_line_dsc_init(&line);
    line.points = path;
    line.base.obj = &chart->obj;
    line.point_cnt = used;
    line.width = width;
    line.color = color;
    line.round_start = 1;
    line.round_end = 1;
    /* Fade the outgoing history without covering the transparent background.
     * Each clipped segment contains two points and a path separator. */
    double fade_width = fmin(48.0, (g->center - g->left) / 3.0);
    uint32_t fading = 0;
    while(fading < used && path[fading].x < g->left + fade_width) {
        double x = (path[fading].x + path[fading + 1].x) * 0.5;
        double alpha = LV_CLAMP(0.0, (x - g->left) / fade_width, 1.0);
        alpha = alpha * alpha * (3.0 - 2.0 * alpha);
        line.points = &path[fading];
        line.point_cnt = 3;
        line.opa = (lv_opa_t)lround(alpha * LV_OPA_COVER);
        lv_draw_line(layer, &line);
        fading += 3;
    }
    line.points = path + fading;
    line.point_cnt = used - fading;
    line.opa = LV_OPA_COVER;
    if(line.point_cnt) lv_draw_line(layer, &line); /* LVGL copies the path into the draw task. */
    lv_free(path);
}

static void chart_event(const lv_obj_class_t * class_p, lv_event_t * e)
{
    if(lv_obj_event_base(class_p, e) != LV_RESULT_OK) return;
    tracking_chart_t * chart = (tracking_chart_t *)lv_event_get_current_target(e);
    if(lv_event_get_code(e) == LV_EVENT_SIZE_CHANGED) {
        refresh_labels(chart);
        return;
    }
    if(lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;
    chart_geometry_t g = geometry(&chart->obj);
    if(g.center <= g.left || g.bottom <= g.top) return;
    lv_area_t coords;
    lv_obj_get_coords(&chart->obj, &coords);
    g.left += coords.x1; g.center += coords.x1; g.right += coords.x1;
    g.top += coords.y1; g.bottom += coords.y1; g.axis += coords.y1;
    lv_layer_t * layer = lv_event_get_layer(e);
    uint32_t now = chart->actual_count ? chart->actual[chart->actual_count - 1].sample.time_ms : 0;
    lv_color_t white = lv_color_hex(0xFFF7EE);
    lv_draw_line_dsc_t guide;
    lv_draw_line_dsc_init(&guide);
    guide.p1 = (lv_point_precise_t){g.center, g.top};
    guide.p2 = (lv_point_precise_t){g.center, coords.y2 - 4};
    guide.color = white;
    guide.opa = 65;
    guide.dash_width = 2;
    guide.dash_gap = 3;
    lv_draw_line(layer, &guide);

    /* Decorative ticks converge at 16 px/s, fading at the center and edges.
     * A five-tick cycle keeps the major marks continuous across phase wraps. */
    const int32_t spacing = 16;
    int32_t span = LV_MIN(g.center - g.left, g.right - g.center);
    double phase = (now % 5000) * spacing / 1000.0;
    for(int32_t i = 0; i <= span / spacing + 5; i++) {
        double distance = i * spacing - phase;
        if(distance <= 0 || distance >= span) continue;
        bool major = i % 5 == 0;
        double fade = fmin(1.0, fmin(distance, span - distance) / spacing);
        lv_opa_t opa = (lv_opa_t)((major ? 145 : 65) * fade);
        int32_t offset = (int32_t)lround(distance);
        draw_line(layer, g.center - offset, g.axis, g.center - offset,
                  g.axis + (major ? 14 : 7), white, opa);
        draw_line(layer, g.center + offset, g.axis, g.center + offset,
                  g.axis + (major ? 14 : 7), white, opa);
    }
    draw_line(layer, g.center, g.axis, g.center, g.axis + 14, white, 170);
    draw_curve(chart, layer, &g, chart->target, chart->target_count, now, lv_color_hex(0xF58A16), 2);
    /* Keep the marker on the target curve at the current time. Draw it below
     * the actual trace so overlapping markers leave the measured value clear. */
    for(uint32_t i = 0; now && i + 1 < chart->target_count; i++) {
        const chart_point_t * a = &chart->target[i];
        const chart_point_t * b = &chart->target[i + 1];
        if(now < a->sample.time_ms || now > b->sample.time_ms) continue;
        double value = interpolate(a, b, now);
        if(value >= chart->y_min && value <= chart->y_max) {
            int32_t y = (int32_t)value_y(chart, &g, value);
            lv_draw_rect_dsc_t dot;
            lv_draw_rect_dsc_init(&dot);
            dot.bg_color = lv_color_hex(0xF58A16);
            dot.bg_opa = LV_OPA_COVER;
            dot.radius = LV_RADIUS_CIRCLE;
            lv_area_t area = {g.center - 5, y - 5, g.center + 4, y + 4};
            lv_draw_rect(layer, &dot, &area);
        }
        break;
    }
    draw_curve(chart, layer, &g, chart->actual, chart->actual_count, now, white, 3);
    if(chart->actual_count) {
        double value = chart->actual[chart->actual_count - 1].sample.value;
        int32_t y = (int32_t)value_y(chart, &g, LV_CLAMP(chart->y_min, value, chart->y_max));
        if(value < chart->y_min || value > chart->y_max) {
            int32_t dy = value > chart->y_max ? 7 : -7;
            draw_line(layer, g.center - 6, y + dy, g.center, y, white, LV_OPA_COVER);
            draw_line(layer, g.center, y, g.center + 6, y + dy, white, LV_OPA_COVER);
        }
        else {
            lv_draw_rect_dsc_t dot;
            lv_draw_rect_dsc_init(&dot);
            dot.bg_color = white;
            dot.bg_opa = LV_OPA_COVER;
            dot.radius = LV_RADIUS_CIRCLE;
            lv_area_t area = {g.center - 7, y - 7, g.center + 6, y + 6};
            lv_draw_rect(layer, &dot, &area);
        }
    }
}

static void chart_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    tracking_chart_t * chart = (tracking_chart_t *)obj;
    lv_free(chart->target);
    lv_free(chart->actual);
}
