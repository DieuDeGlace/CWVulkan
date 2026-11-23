# Android counterparts of the engine's Windows libraries (included from the top-level CMakeLists.txt).
#
# - android_glfw: the part of the GLFW API the engine calls, over ANativeWindow and the app glue's
#   input queue (android/native/AndroidGlfw.cpp). GLFW's own public header declares it.
# - glutess: the SGI polygon tessellator that Windows ships in glu32, built from Mesa's GLU.

# The Windows build's choices where C++ leaves one: plain char is signed there, and MSVC does not
# fuse a multiplication and an addition into one rounding. World generation depends on both.
target_compile_options(engine PUBLIC -fsigned-char -ffp-contract=off)
target_compile_options(deps PUBLIC -fsigned-char -ffp-contract=off)
target_compile_definitions(deps PUBLIC VK_USE_PLATFORM_ANDROID_KHR)
# The owner-data wizard drives the Windows game and its extractor (CubeVulkanSetup only).
get_target_property(engine_sources engine SOURCES)
list(REMOVE_ITEM engine_sources src/OwnerSetup.cpp)
set_property(TARGET engine PROPERTY SOURCES ${engine_sources})

FetchContent_Declare(glfw_headers GIT_REPOSITORY https://github.com/glfw/glfw.git
                     GIT_TAG 3.4 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_Declare(glu GIT_REPOSITORY https://gitlab.freedesktop.org/mesa/glu.git
                     GIT_TAG glu-9.0.3 GIT_SHALLOW TRUE SOURCE_SUBDIR none)
FetchContent_MakeAvailable(glfw_headers glu)

set(native_glue "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
add_library(android_glfw STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/android/native/AndroidGlfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/android/native/TouchControls.cpp
    "${native_glue}/android_native_app_glue.c")
target_include_directories(android_glfw PUBLIC "${glfw_headers_SOURCE_DIR}/include" "${native_glue}"
    ${CMAKE_CURRENT_SOURCE_DIR}/android/native)
target_link_libraries(android_glfw PUBLIC deps android log)

set(tess_sources dict geom memalloc mesh normal priorityq render sweep tess tessmono)
list(TRANSFORM tess_sources PREPEND "${glu_SOURCE_DIR}/src/libtess/")
list(TRANSFORM tess_sources APPEND .c)
add_library(glutess STATIC ${tess_sources})
target_compile_options(glutess PRIVATE -w) # (third-party code, built as it comes)
# GL/gl.h: the few OpenGL types and enumerations the tessellator's header uses (no OpenGL is linked).
target_include_directories(glutess PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/android/native/compat "${glu_SOURCE_DIR}/include")
target_include_directories(glutess PRIVATE "${glu_SOURCE_DIR}/src/include")
