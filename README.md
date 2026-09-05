# LVGL PC 个性化组件开发工程

这是一个用于在 PC 上开发、调试和验证个性化 LVGL UI 组件的工程。通过 SDL2
模拟显示器、鼠标、滚轮和键盘，无需嵌入式硬件即可快速迭代组件的布局、样式与交互。

本项目 fork 自 [lvgl/lv_port_pc_vscode](https://github.com/lvgl/lv_port_pc_vscode)，
并针对独立 UI 组件开发进行了精简和配置调整。

## 当前配置

- 700 × 700 SDL2 模拟窗口
- 32-bit 色深
- 16 ms 默认刷新周期（60 FPS 档）
- SDL2 硬件加速、全帧渲染和双缓冲
- 开启 FPS、系统 CPU、进程 CPU、内存使用和内存碎片率监控
- 开启软件复杂绘制与复杂渐变
- 4 MiB LVGL 内存池，为旋转/缩放 Glow 的中间绘制层预留空间
- 默认不加载任何 demo 或示例 UI
- 默认不构建 LVGL examples、demos 和内置 ThorVG
- 关闭高开销的对象、样式和内存完整性检查

> 16 ms 是 LVGL 使用整数毫秒时最接近 60 FPS 的刷新周期。实际帧率取决于
> UI 复杂度、渲染负载和运行环境。

## 项目结构

```text
.
├── components/          # 个性化 UI 组件
├── src/
│   ├── main.c           # PC 模拟器入口和 UI 创建起点
│   ├── hal/             # SDL2 显示及输入设备初始化
│   └── mouse_cursor_icon.c
├── lv_conf.h            # LVGL 项目配置
├── lvgl/                # LVGL Git 子模块
├── FreeRTOS/            # 可选 FreeRTOS Git 子模块
├── CMakeLists.txt
└── simulator.code-workspace
```

## 环境依赖

需要 C/C++ 编译器、CMake、Make（或其他 CMake 生成器）和 SDL2 开发包。

### Debian / Ubuntu

```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev
```

### Arch Linux

```bash
sudo pacman -S base-devel cmake sdl2
```

### Fedora

```bash
sudo dnf install gcc gcc-c++ cmake make SDL2-devel
```

### macOS

```bash
brew install cmake sdl2
```

Windows 可以通过 [vcpkg](https://github.com/microsoft/vcpkg) 安装 SDL2：

```powershell
vcpkg install sdl2
```

## 获取子模块

首次克隆时建议使用 `--recursive`。如果仓库已经克隆到本地，执行：

```bash
git submodule update --init --recursive
```

## 构建与运行

默认构建组件静态库和所有独立 demo：

```bash
cmake -B build -S .
cmake --build build -j
./bin/tracking_chart_demo
```

可执行文件为 `bin/glow_demo`、`bin/radial_background_demo`、
`bin/glass_button_demo`、`bin/tracking_chart_demo`，窗口统一为 700 × 700。
可单独构建或运行一个 demo：

```bash
cmake --build build --target glow_demo -j
cmake --build build --target run_glow_demo
```

仅构建组件库（不包含 demo、main 或 HAL）：

```bash
cmake -B build-library -S . -DBUILD_COMPONENT_DEMOS=OFF -DBUILD_COMPONENT_TESTS=OFF
cmake --build build-library --target components -j
```

库产物为构建目录中的 `libcomponents.a`（Windows 工具链名称可能不同）。
库依赖 LVGL，当前桌面配置仍需要 SDL2 开发包。

启用已有集成测试：

```bash
cmake -B build -S . -DBUILD_COMPONENT_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 开发个性化组件

按组件名建立目录，CMake 自动发现 `.c`、`.cpp`、`.h` 和 `.hpp` 文件；
`demo/` 以外的文件归入 `components` 静态库，`demo/` 内文件归入独立可执行文件。

```text
components/
└── card/
    ├── card.c
    ├── card.h
    └── demo/
        ├── card_demo.c
        └── card_demo.h
```

组件提供 `card_create(parent)` 等 API。demo 头文件声明入口，源文件实现它：

```c
void card_demo(void)
{
    card_create(lv_screen_active());
}
```

入口固定为 `<组件名>_demo()`，头文件为 `<组件名>/demo/<组件名>_demo.h`。
新增组件无需逐个修改 CMake 源文件列表，构建后生成 `bin/card_demo`。
C++ 实现的入口应在头文件中使用 `extern "C"`，供公共 C 入口调用。

所有 demo 共用 `src/main.c` 的平台初始化与事件循环；FreeRTOS 模式共用
`src/freertos_main.c`。CMake 通过 `DEMO_HEADER` 和 `DEMO_ENTRY` 选择本次
可执行文件的 demo；HAL 和鼠标资源集中在 `simulator_platform` 库。
组件只负责自身行为，demo 负责页面组合，不调用其他组件的 demo。

### Glow Frame

`components/glow/glow.h` 提供透明背景容器 `glow`。它默认铺满父对象，
内部光晕始终位于子对象下方，并被 Frame 边界裁剪：

```c
#include "glow/glow.h"

lv_obj_t * glow = glow_create(parent);
glow_gradient_stop_t stops[] = {
    {lv_color_hex(0xC4B5FD), LV_OPA_80, 0},
    {lv_color_hex(0x818CF8), LV_OPA_50, 96},
    {lv_color_hex(0x3B82F6), LV_OPA_TRANSP, 255},
};
glow_set_gradient(glow, stops, 3);
glow_set_center(glow, -40, 20);
glow_set_eccentricity(glow, 500);
glow_set_angle(glow, 25);
glow_animate_spread(glow, 850, 500, NULL);
```

中心偏移以 Frame 中心为原点，允许超出边界；`spread` 会被限制到 0–1000，
其中 1000 的长轴直径等于 Frame 较长边。离心率限制到 0–999，角度自动归一化到
0–359°。渐变接受 2–4 个按 `position`（0–255）非递减排列的色标；非法输入返回
`LV_RESULT_INVALID` 并保留当前渐变。立即设置扩散范围会取消该 Frame 上正在运行的
扩散动画；动画时传入 `NULL` 缓动函数会使用 ease-in-out。

## 关键配置位置

| 配置 | 文件 | 当前值 |
| --- | --- | --- |
| 模拟窗口尺寸 | `src/main.c` | 700 × 700 |
| 色深 | `lv_conf.h` / `LV_COLOR_DEPTH` | 32 |
| 刷新周期 | `lv_conf.h` / `LV_DEF_REFR_PERIOD` | 16 ms |
| SDL 缓冲数量 | `lv_conf.h` / `LV_SDL_BUF_COUNT` | 2 |
| 性能监控 | `lv_conf.h` / `LV_USE_PERF_MONITOR` | 开启 |
| 内存监控 | `lv_conf.h` / `LV_USE_MEM_MONITOR` | 开启 |

## 可选 CMake 功能

以下功能默认关闭，可在配置阶段按需开启：

```bash
cmake -B build -S . \
  -DLV_USE_DRAW_SDL=ON \
  -DLV_USE_LIBPNG=ON \
  -DLV_USE_LIBJPEG_TURBO=ON \
  -DLV_USE_FFMPEG=ON \
  -DLV_USE_FREETYPE=ON
```

开启前需要先安装对应的系统开发库。

### LVGL Pro 工程

可以通过 `LVGL_PRO_PROJECT_DIR` 接入已有的 LVGL Pro 工程：

```bash
cmake -B build -S . -DLVGL_PRO_PROJECT_DIR=/path/to/lvgl-pro-project
cmake --build build -j
```

### FreeRTOS

如需使用 FreeRTOS，需要同时：

1. 在 `lv_conf.h` 中将 `LV_USE_OS` 设置为 `LV_OS_FREERTOS`。
2. 使用 `USE_FREERTOS` 重新配置工程。

```bash
cmake -B build -S . -DUSE_FREERTOS=ON
cmake --build build -j
```

## VS Code

使用 VS Code 时可以直接打开 `simulator.code-workspace`。完成 CMake 配置后，可通过
CMake Tools 构建，或继续使用终端中的构建命令。

## 上游与第三方项目

- Fork 来源：[lvgl/lv_port_pc_vscode](https://github.com/lvgl/lv_port_pc_vscode)
- LVGL：[lvgl/lvgl](https://github.com/lvgl/lvgl)，当前子模块版本为 **9.6.0-dev**，固定提交为 [`f45e3a3e87f5`](https://github.com/lvgl/lvgl/commit/f45e3a3e87f517ee134d52d01ad7c8a887efd199)。
- SDL2：[libsdl-org/SDL](https://github.com/libsdl-org/SDL)，由系统开发包提供。
- FreeRTOS：[FreeRTOS/FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel)

LVGL、SDL2、FreeRTOS 及其他第三方代码分别遵循其自身许可证。

LVGL 版本号取自 `lvgl/include/lvgl/lv_version.h`；当前使用开发版本，具体代码以仓库记录的子模块提交为准。
