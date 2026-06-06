# CMakeList.txt : CMake project for PlateBlurrer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("PlateBlurrer")

find_package(OpenCV REQUIRED COMPONENTS core dnn imgcodecs imgproc videoio)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable (PlateBlurrer "PlateBlurrer.cpp" "PlateBlurrer.h")

# Link OpenCV and Torch libraries to the executable
target_link_libraries(PlateBlurrer PRIVATE ${OpenCV_LIBRARIES})
target_include_directories(PlateBlurrer PRIVATE ${OpenCV_INCLUDE_DIRS})

if(WIN32)
    set(FFMPEG_DESTDIR "${CMAKE_CURRENT_BINARY_DIR}")
    
    if(NOT EXISTS "${FFMPEG_DESTDIR}/ffmpeg.exe")
        message(STATUS "[All-In-One] Fetching pre-compiled FFmpeg binary for runtime audio pipeline...")
        
        file(DOWNLOAD 
            "https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip"
            "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg.zip"
            SHOW_PROGRESS
        )
        
        message(STATUS "[All-In-One] Extracting FFmpeg...")
        file(ARCHIVE_EXTRACT
            INPUT "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg.zip"
            DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg_extracted"
        )
        
        file(GLOB_RECURSE FFMPEG_EXE "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg_extracted/*ffmpeg.exe")
        if(FFMPEG_EXE)
            file(COPY ${FFMPEG_EXE} DESTINATION "${FFMPEG_DESTDIR}")
        endif()
        
        file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg.zip")
        file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg_extracted")
        message(STATUS "[All-In-One] FFmpeg environment successfully deployed locally.")
    endif()
endif()

# Set modern C++ Standards safely
set_property(TARGET PlateBlurrer PROPERTY CXX_STANDARD 17)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PlateBlurrer PROPERTY CXX_STANDARD 20)
endif()

if(MSVC)
    # Post-build event that copies your runtime dependency DLLs directly to output folder
    add_custom_command(TARGET PlateBlurrer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:PlateBlurrer>
        $<TARGET_FILE_DIR:PlateBlurrer>
        COMMAND_EXPAND_LISTS
    )
endif()