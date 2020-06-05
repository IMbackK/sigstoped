cmake_minimum_required(VERSION 2.4)


project(sigstoped)

set(SRC_FILES main.cpp process.cpp xinstance.cpp)
set(LIBS -lX11)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_link_libraries( ${PROJECT_NAME} ${LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE  "/usr/include/glib-2.0/" "/usr/lib/glib-2.0/include" "/usr/include/gdk-pixbuf-2.0")
set_target_properties( ${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -m64 LINK_FLAGS -m64)
add_definitions(" -std=c++17 -Wall -O2 -flto -fno-strict-aliasing")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
