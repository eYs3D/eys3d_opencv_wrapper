cmake_minimum_required(VERSION 3.10)
project(eys3d_opencv)

if (CMAKE_HOST_WIN32)
    set(WINDOWS 1)
    message("Windows build begin")
endif()

include(cmake/update_git_submodules.cmake)

if(WINDOWS)
    add_compile_options(-fPIC)
    add_definitions("$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14")
    message(CMAKE_CURRENT_LIST_DIR:${CMAKE_CURRENT_LIST_DIR})
    if(Support STREQUAL "opengl")
        add_compile_definitions(SUPPORT_QT_OPENGL)
    endif()
else()
    set(CMAKE_FIND_DEBUG_MODE TRUE)
    set(CMAKE_DEBUG_TARGET_PROPERTIES TRUE)
    add_compile_options(-fPIC)
    add_definitions(-D_DEBUG)
    add_definitions(-DDEBUG)
    add_definitions("$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14")
    message(CMAKE_CURRENT_LIST_DIR:${CMAKE_CURRENT_LIST_DIR})
endif()

## eSPDI dependencies ##
if(WINDOWS)
    include_directories(
            ${CMAKE_CURRENT_LIST_DIR}/include/
            ${CMAKE_CURRENT_LIST_DIR}/eSPDI_win64/
    )
else()
    include_directories(
            ${CMAKE_CURRENT_LIST_DIR}/include/
            ${CMAKE_CURRENT_LIST_DIR}/eSPDI/
            ${CMAKE_CURRENT_LIST_DIR}/libeYs3D_linux/magic/
            ${CMAKE_CURRENT_LIST_DIR}/libeYs3D_linux/include/
            ${CMAKE_CURRENT_LIST_DIR}/libeYs3D_linux/include/DMPreview_utility/
    )
endif()

if(WINDOWS)
    link_directories(
            ${CMAKE_CURRENT_LIST_DIR}/eSPDI_win64/
    )
else()
    link_directories(
            ${CMAKE_CURRENT_LIST_DIR}/eSPDI/
            ${CMAKE_CURRENT_LIST_DIR}/libeYs3D_linux/lib
    )

endif()

if(WINDOWS)
    set(opencv_root "${CMAKE_CURRENT_LIST_DIR}/opencv_libs/opencv-3.4.2_vc")
    set(opencv_include ${opencv_root}/include)
    set(opencv_lib_dir ${opencv_root}/lib)
    set(opencv_libs opencv_world342.lib freeglut.lib glew32s.lib)

    include_directories(${opencv_include})
    link_directories(${opencv_lib_dir})
    link_libraries(${opencv_libs})

    ## eSPDI dependencies ##
    set(eys3d eys3d.lib)
    include_directories(
            ${CMAKE_CURRENT_LIST_DIR}/include/
            ${CMAKE_CURRENT_LIST_DIR}/eSPDI_win64/include
            ${CMAKE_CURRENT_LIST_DIR}/libeYs3D/magic/
            ${CMAKE_CURRENT_LIST_DIR}/libeYs3D/include/
            ${CMAKE_CURRENT_LIST_DIR}/libeYs3D/include/DMPreview_utility/
            ${CMAKE_CURRENT_LIST_DIR}/libeYs3D/include/devices/
    )

    link_directories(
            ${CMAKE_CURRENT_LIST_DIR}/libeYs3D/lib
    )

    link_libraries(${eys3d})

else()
    set(CMAKE_INSTALL_RPATH $ORIGIN)
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH}:$ORIGIN/../eSPDI)
    if(Support STREQUAL "default")
        message("system default path")
        find_package(OpenCV REQUIRED)
    elseif(Support STREQUAL "gtk")
        message("support gtk")
        set(OPENCV_PATH ${CMAKE_CURRENT_LIST_DIR}/opencv_libs/opencv3.4.2_gtk)
        message(OPENCV_PATH:${OPENCV_PATH})
        find_package(OpenCV PATHS ${OPENCV_PATH} NO_DEFAULT_PATH REQUIRED)
        set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH}:$ORIGIN/../opencv_libs/opencv3.4.2_gtk/lib/)
    elseif(Support STREQUAL "opengl")
        message("support opengl")
        set(OPENCV_PATH ${CMAKE_CURRENT_LIST_DIR}/opencv_libs/opencv3.4.2_QT_GL)
        message(OPENCV_PATH:${OPENCV_PATH})
        find_package(OpenCV PATHS ${OPENCV_PATH} NO_DEFAULT_PATH REQUIRED)
        set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH}:$ORIGIN/../opencv_libs/opencv3.4.2_QT_GL/lib/)
        add_definitions("-DSUPPORT_QT_OPENGL")
        find_package(OpenGL REQUIRED)
        find_package(GLUT REQUIRED)
        set(DEPENDENCIES ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${DEPENDENCIES})
    endif()

    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/modules)
    message(CMAKE_MODULE_PATH:${CMAKE_MODULE_PATH})

    # Third party
    find_package(UDev REQUIRED)
    message(UDEV_LIBRARIES:${UDEV_LIBRARIES} UDEV_INCLUDE_DIRS:${UDEV_INCLUDE_DIRS})
    include_directories(${UDEV_INCLUDE_DIRS})
    ## ##
endif()

## source directory ##
message(DEPENDENCIES:${DEPENDENCIES})

#aux_source_directory(src DIR_SRCS)
if(WINDOWS)
    set(DIR_SRCS
        src/main_win.cpp)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/x64)
else()
    set(DIR_SRCS
        src/linux/main.cpp
        src/linux/exampleBinder.cpp
        )
endif()
add_executable(eys3d_opencv ${DIR_SRCS} include/main.h)

if(WINDOWS)
    target_link_libraries(eys3d_opencv ${eys3d} ${opencv_libs} ${DEPENDENCIES})
    target_precompile_headers(eys3d_opencv PRIVATE eSPDI_win64/include/stdafx.h)
    install(
            TARGETS eys3d_opencv
            RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/x64
    )
    file(COPY eSPDI_win64/lib/eSPDI_DM.dll  # This lib have merged tj lib in it.
            libeYs3D/lib/eys3d.dll
            DESTINATION ${CMAKE_BINARY_DIR}/x64/Release)
    add_custom_command(TARGET eys3d_opencv POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                           ${CMAKE_SOURCE_DIR}/asset $<TARGET_FILE_DIR:eys3d_opencv>/asset)
    add_custom_command(TARGET eys3d_opencv POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                           ${CMAKE_SOURCE_DIR}/cfg/ $<TARGET_FILE_DIR:eys3d_opencv>/eYs3D/cfg)
    add_custom_command(TARGET eys3d_opencv POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                           ${CMAKE_SOURCE_DIR}/opencv_libs/opencv-3.4.2_vc/share/OpenCV/haarcascades/ $<TARGET_FILE_DIR:eys3d_opencv>/haarcascades)
    file(GLOB VTK_FILES   "${CMAKE_SOURCE_DIR}/opencv_libs/opencv-3.4.2_vc/lib/VTK/*.dll")
    file(COPY ${VTK_FILES} DESTINATION ${CMAKE_BINARY_DIR}/x64/Release)
    file(GLOB DLL_FILES   "${CMAKE_SOURCE_DIR}/opencv_libs/opencv-3.4.2_vc/lib/*.dll")
    file(COPY ${DLL_FILES} DESTINATION ${CMAKE_BINARY_DIR}/x64/Release)
    file(COPY ModeConfig.db DESTINATION ${CMAKE_BINARY_DIR}/x64/Release/eYs3D/cfg)
else()
    target_link_libraries(eys3d_opencv eys3d pthread ${UDEV_LIBRARIES} ${OpenCV_LIBS} ${DEPENDENCIES})
    install(
            TARGETS eys3d_opencv
            RUNTIME DESTINATION ${CMAKE_CURRENT_LIST_DIR}/bin
    )
endif()


