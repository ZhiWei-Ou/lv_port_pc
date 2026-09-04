#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "lvgl/lvgl.h"
#include "hal/hal.h"

#define WINDOW_WIDTH  720
#define WINDOW_HEIGHT 720

#if LV_USE_OS != LV_OS_FREERTOS

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    lv_init();
    sdl_hal_init(WINDOW_WIDTH, WINDOW_HEIGHT);

    /* Add UI objects to lv_screen_active() here. */

    while(1) {
        uint32_t sleep_time_ms = lv_timer_handler();
        if(sleep_time_ms == LV_NO_TIMER_READY) {
            sleep_time_ms = LV_DEF_REFR_PERIOD;
        }
#ifdef _MSC_VER
        Sleep(sleep_time_ms);
#else
        usleep(sleep_time_ms * 1000);
#endif
    }

    return 0;
}

#endif
