#ifndef GLASS_BUTTON_H
#define GLASS_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/* Creates a 64 px circular button. Unequal width/height produces a capsule.
 * Add labels, images or other content directly as children; all content scales
 * with the button. Decorative children should not be clickable.
 * The translucent surface simulates glass; it does not blur the backdrop.
 */
lv_obj_t * ui_glass_button_create(lv_obj_t * parent);

#ifdef __cplusplus
}
#endif

#endif /* GLASS_BUTTON_H */
