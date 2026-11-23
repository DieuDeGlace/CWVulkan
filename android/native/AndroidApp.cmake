# The Android app's native library (included from the top-level CMakeLists.txt): src/main.cpp as
# cubevulkanMain, started by android/native/AndroidMain.cpp on NativeActivity's thread.
add_library(CubeVulkan SHARED src/main.cpp android/native/AndroidMain.cpp)
set_target_properties(CubeVulkan PROPERTIES OUTPUT_NAME cubevulkan)
target_link_libraries(CubeVulkan PRIVATE engine)
# android_main is reached through the glue's ANativeActivity_onCreate, which nothing references.
target_link_options(CubeVulkan PRIVATE -u ANativeActivity_onCreate)
