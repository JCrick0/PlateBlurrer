# CMakeList.txt : CMake project for PlateBlurrer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("PlateBlurrer")

find_package(OpenCV REQUIRED)

add_executable (PlateBlurrer "PlateBlurrer.cpp" "PlateBlurrer.h")

# Link OpenCV and Torch libraries to the executable
target_link_libraries(PlateBlurrer PRIVATE ${OpenCV_LIBS} ${TORCH_LIBRARIES})
target_include_directories(PlateBlurrer PRIVATE ${OpenCV_INCLUDE_DIRS})

set_property(TARGET PlateBlurrer PROPERTY CXX_STANDARD 17)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PlateBlurrer PROPERTY CXX_STANDARD 20)
endif()

if(MSVC)
    # Post-build event that tells CMake to parse your executable,
    # finds every DLL it depends on from vcpkg, and copies them to output directory.
    add_custom_command(TARGET PlateBlurrer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:PlateBlurrer>
        $<TARGET_FILE_DIR:PlateBlurrer>
        COMMAND_EXPAND_LISTS
    )
endif()