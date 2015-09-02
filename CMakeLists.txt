cmake_minimum_required(VERSION 3.1.0)
project(OSVR-Optical-Calibration) # Change this line.

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(osvr REQUIRED)
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
#find_package(GLEW REQUIRED)

# Add the libSDL2pp subproject
#add_subdirectory(vendor/libSDL2pp)

add_executable(osvr-optical-calib SDL2Helpers.h OpticalCalib.cpp)
target_link_libraries(osvr-optical-calib
    PRIVATE
    osvr::osvrClientKitCpp
    #${SDL2PP_LIBRARIES}
    ${OPENGL_LIBRARY}
    SDL2::SDL2main)
#    GLEW::GLEW)
target_include_directories(osvr-optical-calib
    PRIVATE
    #${SDL2PP_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm/")

#if(SDL2_DYNAMIC)
#    osvr_copy_dep(OpenGLSample SDL2::SDL2)
#endif()
