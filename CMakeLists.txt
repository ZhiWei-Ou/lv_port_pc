cmake_minimum_required(VERSION 3.12.4)
project(lvgl C CXX)

# Set the correct FreeRTOS port for your system (e.g., Posix for WSL)
set(FREERTOS_PORT GCC_POSIX CACHE STRING "Port for FreeRTOS on Posix environment")

set(LVGL_PRO_PROJECT_DIR
    ""
    CACHE PATH "Path to LVGL Pro Project folder")

option(USE_FREERTOS "Enable FreeRTOS" OFF) # Turn this on to enable FreeRTOS

if(USE_FREERTOS)
    message(STATUS "FreeRTOS is enabled")

    # FreeRTOS integration when USE_FREERTOS is enabled
    add_library(freertos_config INTERFACE)
    target_include_directories(freertos_config SYSTEM INTERFACE ${PROJECT_SOURCE_DIR}/config)
    target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)

    # Add FreeRTOS as a subdirectory
    add_subdirectory(FreeRTOS)

    # FreeRTOS-specific include directories
    include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS/include)
    include_directories(${PROJECT_SOURCE_DIR}/FreeRTOS/portable/ThirdParty/GCC/Posix)
    include_directories(${PROJECT_SOURCE_DIR}/config)

    # Add FreeRTOS sources
    file(GLOB FREERTOS_SOURCES
        "${PROJECT_SOURCE_DIR}/FreeRTOS/*.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/portable/MemMang/heap_4.c"
        "${PROJECT_SOURCE_DIR}/FreeRTOS/portable/ThirdParty/GCC/Posix/*.c"
    )
else()
    message(STATUS "FreeRTOS is disabled")
    set(FREERTOS_SOURCES "")  # No FreeRTOS sources if FreeRTOS is disabled
endif()

# Define options for LVGL with default values (OFF)
option(LV_USE_DRAW_SDL "Use SDL draw unit" OFF)
option(LV_USE_LIBPNG "Use libpng to decode PNG" OFF)
option(LV_USE_LIBJPEG_TURBO "Use libjpeg turbo to decode JPEG" OFF)
option(LV_USE_FFMPEG "Use libffmpeg to display video using lv_ffmpeg" OFF)
option(LV_USE_FREETYPE "Use freetype library" OFF)

# Set C and C++ standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the output path for the executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# Find and include SDL2 library
find_package(SDL2 REQUIRED)

# Add compile definitions based on the selected options
add_compile_definitions($<$<BOOL:${LV_USE_DRAW_SDL}>:LV_USE_DRAW_SDL=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBPNG}>:LV_USE_LIBPNG=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBJPEG_TURBO}>:LV_USE_LIBJPEG_TURBO=1>)
add_compile_definitions($<$<BOOL:${LV_USE_FFMPEG}>:LV_USE_FFMPEG=1>)

# Add LVGL subdirectory
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Build LVGL examples" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Build LVGL demos" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "Build bundled ThorVG" FORCE)
add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${SDL2_INCLUDE_DIRS})

set(PLATFORM_LIBS lvgl ${SDL2_LIBRARIES})
if(USE_FREERTOS)
    list(APPEND PLATFORM_LIBS freertos_config freertos_kernel)
endif()
if(NOT MSVC)
    list(APPEND PLATFORM_LIBS m pthread)
endif()

# Conditionally include and link SDL2_image if LV_USE_DRAW_SDL is enabled
if(LV_USE_DRAW_SDL)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
    find_package(SDL2_image REQUIRED)
    target_include_directories(lvgl PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
    list(APPEND PLATFORM_LIBS ${SDL2_IMAGE_LIBRARIES})
endif()

# Conditionally include and link libpng if LV_USE_LIBPNG is enabled
if(LV_USE_LIBPNG)
    find_package(PNG REQUIRED)
    target_include_directories(lvgl PUBLIC ${PNG_INCLUDE_DIRS})
    list(APPEND PLATFORM_LIBS ${PNG_LIBRARIES})
endif()

# Conditionally include and link libjpeg-turbo if LV_USE_LIBJPEG_TURBO is enabled
if(LV_USE_LIBJPEG_TURBO)
    find_package(JPEG REQUIRED)
    target_include_directories(lvgl PUBLIC ${JPEG_INCLUDE_DIRS})
    list(APPEND PLATFORM_LIBS ${JPEG_LIBRARIES})
endif()

# Conditionally include and link FFmpeg libraries if LV_USE_FFMPEG is enabled
if(LV_USE_FFMPEG)
    list(APPEND PLATFORM_LIBS avformat avcodec avutil swscale z)
endif()

# Conditionally include and link FreeType if LV_USE_FREETYPE is enabled
if(LV_USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(lvgl PUBLIC ${FREETYPE_INCLUDE_DIRS})
    list(APPEND PLATFORM_LIBS ${FREETYPE_LIBRARIES})
endif()

if(LVGL_PRO_PROJECT_DIR)
  if(EXISTS "${LVGL_PRO_PROJECT_DIR}/CMakeLists.txt")
    add_subdirectory(${LVGL_PRO_PROJECT_DIR} ui)
    list(APPEND PLATFORM_LIBS lib-ui)

    target_compile_definitions(lib-ui PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)
    target_include_directories(lib-ui PUBLIC
      ${LVGL_PRO_PROJECT_DIR}  # local headers like ui.h, generated files, etc.
      ${CMAKE_SOURCE_DIR}/lvgl # include "lvgl.h" works
    )
    message(STATUS "LVGL Pro Project enabled from: ${LVGL_PRO_PROJECT_DIR}")
  else()
    message(
      WARNING
        "LVGL_PRO_PROJECT_DIR specified but no CMakeLists.txt found at: ${LVGL_PRO_PROJECT_DIR}"
    )
  endif()
endif()

# Component implementation files belong to the library; demos are separate apps.
file(GLOB_RECURSE COMPONENT_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/components/*.c"
    "${PROJECT_SOURCE_DIR}/components/*.cpp"
    "${PROJECT_SOURCE_DIR}/components/*.h"
    "${PROJECT_SOURCE_DIR}/components/*.hpp")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "/demo/")
add_library(components STATIC ${COMPONENT_SOURCES})
target_compile_definitions(components PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_include_directories(components PUBLIC ${PROJECT_SOURCE_DIR}/components)
target_link_libraries(components PUBLIC lvgl)
if(NOT MSVC)
    target_link_libraries(components PUBLIC m)
endif()

option(BUILD_COMPONENT_DEMOS "Build each component demo executable" ON)
option(BUILD_COMPONENT_TESTS "Build desktop component integration tests" OFF)
if(BUILD_COMPONENT_DEMOS OR BUILD_COMPONENT_TESTS)
    add_library(simulator_platform STATIC src/hal/hal.c src/mouse_cursor_icon.c)
    target_include_directories(simulator_platform PUBLIC ${PROJECT_SOURCE_DIR}/src)
    target_compile_definitions(simulator_platform PUBLIC LV_CONF_INCLUDE_SIMPLE)
    target_link_libraries(simulator_platform PUBLIC ${PLATFORM_LIBS})
    if(USE_FREERTOS)
        target_sources(simulator_platform PRIVATE src/freertos/freertos_posix_port.c ${FREERTOS_SOURCES})
    endif()
endif()

if(BUILD_COMPONENT_DEMOS)
    if(USE_FREERTOS)
        set(DEMO_MAIN src/freertos_main.c)
    else()
        set(DEMO_MAIN src/main.c)
    endif()
    file(GLOB COMPONENT_DIRS CONFIGURE_DEPENDS LIST_DIRECTORIES true "${PROJECT_SOURCE_DIR}/components/*")
    foreach(COMPONENT_DIR IN LISTS COMPONENT_DIRS)
        if(NOT IS_DIRECTORY "${COMPONENT_DIR}/demo")
            continue()
        endif()
        get_filename_component(COMPONENT_NAME "${COMPONENT_DIR}" NAME)
        set(DEMO_NAME "${COMPONENT_NAME}_demo")
        file(GLOB_RECURSE DEMO_SOURCES CONFIGURE_DEPENDS
            "${COMPONENT_DIR}/demo/*.c" "${COMPONENT_DIR}/demo/*.cpp"
            "${COMPONENT_DIR}/demo/*.h" "${COMPONENT_DIR}/demo/*.hpp")
        add_executable(${DEMO_NAME} ${DEMO_MAIN} ${DEMO_SOURCES})
        target_compile_definitions(${DEMO_NAME} PRIVATE
            DEMO_HEADER="${COMPONENT_NAME}/demo/${DEMO_NAME}.h"
            DEMO_ENTRY=${DEMO_NAME})
        target_link_libraries(${DEMO_NAME} PRIVATE components simulator_platform)
        add_custom_target(run_${DEMO_NAME}
            COMMAND $<TARGET_FILE:${DEMO_NAME}> DEPENDS ${DEMO_NAME}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} USES_TERMINAL)
        if(WIN32)
            if(MSVC)
                target_link_options(${DEMO_NAME} PRIVATE "/SUBSYSTEM:CONSOLE")
            else()
                target_link_options(${DEMO_NAME} PRIVATE "-mconsole")
            endif()
        endif()
    endforeach()
endif()

if(BUILD_COMPONENT_TESTS AND NOT USE_FREERTOS)
    enable_testing()
    add_executable(tracking_chart_test
        tests/tracking_chart_test.c
        components/tracking_chart/demo/tracking_chart_demo.c)
    target_include_directories(tracking_chart_test PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(tracking_chart_test PRIVATE components simulator_platform)
    if(NOT MSVC)
        target_compile_options(tracking_chart_test PRIVATE -Wall -Wextra -Werror)
    endif()
    add_test(NAME tracking_chart COMMAND tracking_chart_test)
    add_test(NAME tracking_chart_sdl_stop COMMAND tracking_chart_test --sdl)
    set_tests_properties(tracking_chart_sdl_stop PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software")
endif()

# Apply additional compile options if the build type is Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug mode enabled")

    target_compile_options(lvgl PRIVATE
        -pedantic-errors
        -Wall
        -Wclobbered
        -Wdeprecated
        -Wdouble-promotion
        -Wempty-body
        -Wextra
        -Wformat-security
        -Wmaybe-uninitialized
        # -Wmissing-prototypes
        -Wpointer-arith
        -Wmultichar
        -Wno-pedantic # ignored for now, we convert functions to pointers for properties table.
        -Wreturn-type
        -Wshadow
        -Wshift-negative-value
        -Wsizeof-pointer-memaccess
        -Wtype-limits
        -Wundef
        -Wuninitialized
        -Wunreachable-code
        -Wfloat-conversion
        -Wstrict-aliasing
    )

if (ASAN)
    message(STATUS "AddressSanitizer enabled")

    # Add AddressSanitizer flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
else()
    message(STATUS "AddressSanitizer disabled")
endif()
endif()
