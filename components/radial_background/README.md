# Radial background

静态同心圆渐变背景，由 LVGL 直接绘制，不加载图片，不创建动画或定时器。

接口位于 `radial_background.h`，用法见 `demo/radial_background_demo.c`。

- `radial_background_create(parent)`：默认填满父容器，置于子对象底层，不接收点击。可用 LVGL 的位置和尺寸接口调整绘制区域。
- `radial_background_set_center(obj, x, y)`：圆心相对组件左上角，单位像素，默认 `(0, 0)`，允许位于组件之外。
- `radial_background_set_stops(obj, stops, count)`：设置颜色和各自的像素半径。半径必须非负且严格递增；第一半径以内保持第一种颜色，最后半径以外保持最后一种颜色，相邻半径之间线性渐变。
- 修改任意颜色或半径后重新设置 stops。数组由组件复制，调用后无需保留。支持单色；尚未设置颜色时透明。无效输入或分配失败返回 `LV_RESULT_INVALID` 并保留原配置。
- 依赖已启用的 `LV_USE_DRAW_SW_COMPLEX_GRADIENTS`。每对相邻颜色使用一次渐变绘制，不受 `LV_GRADIENT_MAX_STOPS` 限制；重绘成本随颜色数量和绘制面积增加。

桌面入口默认启动 700 × 700 的 Demo：圆心 `(350, 350)`，颜色为 `#AD4902`、`#510900`、`#141414`，半径分别为 `39`、`233`、`467 px`。

构建：`cmake -B build -S . && cmake --build build -j`，运行：`./bin/radial_background_demo`。

验证：构建通过；LVGL 内存显示验证了中心、色阶和角落颜色，以及圆心移动、六色、单色、无效输入保留原配置和对象销毁。已检查导出的 720 × 720 渲染效果。内存显示的监视器读数不代表 SDL 桌面性能。
