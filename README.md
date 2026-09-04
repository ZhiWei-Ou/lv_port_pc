# LVGL PC 个性化组件开发工程

这是一个用于在 PC 上开发、调试和验证个性化 LVGL UI 组件的工程。通过 SDL2
模拟显示器、鼠标、滚轮和键盘，无需嵌入式硬件即可快速迭代组件的布局、样式与交互。

本项目 fork 自 [lvgl/lv_port_pc_vscode](https://github.com/lvgl/lv_port_pc_vscode)，
并针对独立 UI 组件开发进行了精简和配置调整。

## 当前配置

- 720 × 720 SDL2 模拟窗口
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

在项目根目录执行：

```bash
cmake -B build -S .
cmake --build build -j
./bin/main
```

也可以使用项目提供的 CMake `run` target：

```bash
cmake --build build --target run
```

程序启动后显示一个 720 × 720 的空白 screen。右下角显示 FPS 与 CPU 信息，
左下角显示内存使用和碎片率。

## 开发个性化组件

自定义组件统一放在 `components/` 下。建议一个组件使用一组 `.c` 和 `.h` 文件：

```text
components/
├── ui_card.c
├── ui_card.h
├── ui_button.c
├── ui_button.h
└── ui_components.h
```

组件 API 建议接收父对象并返回创建出的根对象：

```c
lv_obj_t * ui_card_create(lv_obj_t * parent);
```

新增组件后，在 `add_executable(main ...)` 之后将源文件和头文件目录加入目标：

```cmake
target_sources(main PRIVATE
    components/ui_card.c
)

target_include_directories(main PRIVATE
    ${PROJECT_SOURCE_DIR}/components
)
```

然后在 `src/main.c` 的 UI 创建位置使用当前 screen：

```c
ui_card_create(lv_screen_active());
```

组件应尽量只负责自身结构、样式和内部事件。screen 级布局、页面切换和业务状态建议
由上层 UI 代码管理，使组件能够在不同页面中复用。

### Glow Frame

`components/glow/glow.h` 提供透明背景容器 `ui_glow`。它默认铺满父对象，
内部光晕始终位于子对象下方，并被 Frame 边界裁剪：

```c
#include "glow/glow.h"

lv_obj_t * glow = ui_glow_create(parent);
ui_glow_gradient_stop_t stops[] = {
    {lv_color_hex(0xC4B5FD), LV_OPA_80, 0},
    {lv_color_hex(0x818CF8), LV_OPA_50, 96},
    {lv_color_hex(0x3B82F6), LV_OPA_TRANSP, 255},
};
ui_glow_set_gradient(glow, stops, 3);
ui_glow_set_center(glow, -40, 20);
ui_glow_set_eccentricity(glow, 500);
ui_glow_set_angle(glow, 25);
ui_glow_animate_spread(glow, 850, 500, NULL);
```

中心偏移以 Frame 中心为原点，允许超出边界；`spread` 会被限制到 0–1000，
其中 1000 的长轴直径等于 Frame 较长边。离心率限制到 0–999，角度自动归一化到
0–359°。渐变接受 2–4 个按 `position`（0–255）非递减排列的色标；非法输入返回
`LV_RESULT_INVALID` 并保留当前渐变。立即设置扩散范围会取消该 Frame 上正在运行的
扩散动画；动画时传入 `NULL` 缓动函数会使用 ease-in-out。

## 关键配置位置

| 配置 | 文件 | 当前值 |
| --- | --- | --- |
| 模拟窗口尺寸 | `src/main.c` | 720 × 720 |
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
- LVGL：[lvgl/lvgl](https://github.com/lvgl/lvgl)
- SDL：[libsdl-org/SDL](https://github.com/libsdl-org/SDL)
- FreeRTOS：[FreeRTOS/FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel)

LVGL、SDL2、FreeRTOS 及其他第三方代码分别遵循其自身许可证。
