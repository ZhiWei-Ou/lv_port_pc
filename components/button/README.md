# Glass button

圆形或胶囊形半透明玻璃质感按钮，左上与右下有渐隐边缘高光。背景采用低透明度暖白色叠加，能透出下方内容；本组件不模糊背景。当前 LVGL 的 backdrop blur 在独立缩放层内无法直接取得下层场景，因此这里采用透明表面与高光组合，保持按压前后外观一致。

```c
#include "button/glass_button.h"

lv_obj_t * button = glass_button_create(parent);
lv_obj_set_size(button, 72, 72);
lv_obj_center(button);

lv_obj_t * label = lv_label_create(button);
lv_label_set_text(label, "Start");
lv_obj_center(label);
```

图片同样直接使用按钮作为 parent：

```c
lv_obj_t * image = lv_image_create(button);
lv_image_set_src(image, &my_image);
lv_obj_center(image);
```

- 默认直径 64 px；宽高不同时呈胶囊形，高光及点击区域随尺寸适配。内容布局由调用方决定。
- 按下用 140 ms 缩至约 95%，松开或拖出用 180 ms 恢复；采用 ease-in-out 过渡，快速重复按压从当前缩放值继续。LVGL 的缩放精度为 1/256，实际按下比例为 243/256。
- 按钮背景、高光及所有子控件一起缩放，不改变布局尺寸；缩放中心位于按钮中心。
- 标签和图片默认可以直接使用。普通 `lv_obj` 等装饰性子控件需调用 `lv_obj_set_clickable(child, false)`，避免拦截按钮输入；若放入独立交互控件，其输入仍由该子控件处理。
- 用标准 `lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, user_data)` 处理点击。圆角外不响应，默认拖出取消点击。支持标准禁用状态和键盘焦点外框。
- 使用标准 `bg_color` / `bg_opa` 样式调整玻璃底色及透明度；请勿用整体 `opa` 调整背景透明度，否则内容也会变淡。

桌面入口启动 700 × 700 Demo，在径向渐变背景上展示 320 × 72 的长条按钮。按住至 LVGL 识别长按后即可拖动，松开停在当前位置并恢复大小。短按不会移动；拖动时捕获指针，按钮限制在窗口范围内。拖动逻辑位于 Demo，普通组件不自动开启拖动。若为这个可拖动 Demo 增加短按动作，可监听 `LV_EVENT_SHORT_CLICKED`，避免长按拖动后触发该动作。执行 `cmake --build build -j` 后运行 `./bin/glass_button_demo`。

长条 Demo 验证：构建及严格警告编译通过；模拟指针验证短按不移动、长按后位移、快速移出仍跟随、窗口边界限制、松开恢复且保留位置、胶囊直边命中和圆角外不响应，以及拖动期间删除。已检查静止及拖动截图。

长按生效时文字切换为 `Drag to move`，松开恢复 `Hold to drag`。拖动位置由抓取起点与指针累计位移计算，不依赖尚未更新的布局坐标。已通过真实 SDL 鼠标驱动的事件队列回归验证（dummy 显示、software renderer）：单次移动、同一帧连续 20 个移动事件、重复读取和松开后的最终位置均正确。

验证：CMake 构建通过；组件及 Demo 以 `-Wall -Wextra -Werror` 编译通过。使用 LVGL 内存显示和模拟指针验证按压/恢复中间帧及终点、快速反向、拖出取消、圆外命中、文字与图片子对象输入、禁用以及动画期间删除。松开后的按钮区域与按下前逐像素一致；已检查静止和按下截图。尚未测量桌面 SDL 的实际 FPS。
