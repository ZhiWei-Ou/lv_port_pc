# Segmented switch

双选项胶囊开关，默认 320 × 64 像素。左侧为 checked（默认 ON），右侧为 unchecked（默认 OFF）。点击某一侧选择该侧；重复点击当前选项不触发变化。键盘左右键选择，确认键切换。

```c
lv_obj_t * control = segmented_switch_create(parent);
segmented_switch_set_labels(control, "ON", "OFF");
segmented_switch_set_checked(control, false, true); // 最后一个参数控制动画
bool enabled = segmented_switch_get_checked(control);
lv_obj_add_event_cb(control, on_changed, LV_EVENT_VALUE_CHANGED, NULL);
```

滑块过渡为 220 ms 缓入缓出，快速反向从当前位置继续；删除组件时 LVGL 清理动画。用户操作仅在值改变时发出 `LV_EVENT_VALUE_CHANGED`，程序 setter 不发事件。

使用 `lv_obj_set_size()`、`lv_obj_align()` 调整布局，使用 `LV_STATE_DISABLED` 禁用。选择值通过组件 setter 修改，以同步滑块和 checked 状态。标签由组件持有并复制文本；内部子对象由组件管理。

运行：`./bin/segmented_switch_demo`。
