# Value button

带左侧标题和右侧 value 的胶囊按钮，复用 glass button 的表面、反光和按压缩放。默认 320 × 64 像素，value 占内容宽度的 30%，长文本省略显示。

```c
lv_obj_t * button = value_button_create(parent);
value_button_set_title(button, "ROUTINE 1");
value_button_set_value(button, "ON");
lv_obj_add_event_cb(button, on_clicked, LV_EVENT_CLICKED, NULL);
```

文本复制存储，`value_button_get_value()` 返回组件持有的字符串，更新或删除后失效。组件不会自动修改 value；点击后如何处理由调用方决定。可通过 LVGL 的尺寸、位置、样式、`LV_STATE_DISABLED` 配置按钮。内部两个标签由组件管理，不应删除、重排或替换。

运行：`./bin/value_button_demo`。前两行点击切换 ON/OFF，第三行展示禁用状态。
