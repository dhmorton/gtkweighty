cmake_minimum_required(VERSION 3.28.0)
project(gtkweighty VERSION 0.1.0 LANGUAGES C)

file(GLOB SOURCES
	src/*.h
	src/*.c
)

#use pgkconfig to find gtk+ headers
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(gtkweighty ${SOURCES})
target_link_libraries(gtkweighty PRIVATE ${GTK3_LIBRARIES} m)
target_link_libraries(gtkweighty PUBLIC -lgio-2.0 -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -ldl -lm -lgtk-3 -lgdk-3 -lcurl -lxml2 -lgdk_pixbuf-2.0 -lImlib2)

target_include_directories(gtkweighty PRIVATE ${GTK3_INCLUDE_DIRS})
target_link_directories(gtkweighty PRIVATE ${GTK3_LIBRARY_DIRS})
target_compile_options(gtkweighty PRIVATE ${GTK3_CFLAGS_OTHER})