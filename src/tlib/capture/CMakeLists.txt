cmake_minimum_required(VERSION 3.18)
project(telecap)

set(CMAKE_CXX_STANDARD 14)

#find the packages make sure it exist
find_package(glfw3 3.3 REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(X11 REQUIRED)

add_executable(client test_client.cpp capture_x11.cpp)

#make the link
# link opgl stuff
target_link_libraries(client glfw)
target_link_libraries(client OpenGL)
target_link_libraries(client GLEW)

# the x11 files
target_link_libraries(client X11)
target_link_libraries(client Xext)
target_link_libraries(client Xrandr)