cmake_minimum_required(VERSION 3.16)
project(gtk-made-qt VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

include(GNUInstallDirs)

set(GTKQT_QT_MAJOR 6 CACHE STRING "Qt major version to build against (6 or 5)")
option(GTKQT_BUILD_EXAMPLES "Build the GTK example programs against the shim" ON)
option(GTKQT_BUILD_TESTS "Build the test suite" ON)

find_package(Qt${GTKQT_QT_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)
# GLib/GObject/GIO are plain C utility libraries (not GTK). Qt itself already
# runs on the GLib main loop on Linux, so they come along for free.
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0 gobject-2.0 gio-2.0 gmodule-2.0)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

add_library(gtk-made-qt SHARED
    src/core.cpp
    src/containers.cpp
    src/controls.cpp
    src/range.cpp
    src/menus.cpp
    src/text.cpp
    src/dialogs.cpp
    src/application.cpp
    src/drawing.cpp
    src/treeview.cpp
    src/builder.cpp
    src/uimanager.cpp
)
target_compile_definitions(gtk-made-qt PRIVATE GTKQT_COMPILATION QT_NO_KEYWORDS)
target_include_directories(gtk-made-qt PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/gtk-made-qt>)
target_link_libraries(gtk-made-qt PUBLIC PkgConfig::GLIB PkgConfig::CAIRO PRIVATE Qt::Widgets)
set_target_properties(gtk-made-qt PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 0)

install(TARGETS gtk-made-qt LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gtk-made-qt)

# Drop-in pkg-config files. Pointing PKG_CONFIG_PATH at one of these
# directories makes "pkg-config gtk+-3.0" (etc.) resolve to the shim.
#   build/pkgconfig/          -> use straight from the build tree
#   <prefix>/lib/gtk-made-qt/pkgconfig -> installed copy
foreach(api 2 3 4)
    if(api EQUAL 2)
        set(PCNAME gtk+-2.0)
        set(GTK_FAKE_VERSION 2.24.33)
    elseif(api EQUAL 3)
        set(PCNAME gtk+-3.0)
        set(GTK_FAKE_VERSION 3.24.41)
    else()
        set(PCNAME gtk4)
        set(GTK_FAKE_VERSION 4.14.0)
    endif()
    set(GTKQT_API ${api})

    set(PC_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set(PC_LIBDIR ${CMAKE_CURRENT_BINARY_DIR})
    set(PC_EXTRA_LIBS "-Wl,-rpath,${CMAKE_CURRENT_BINARY_DIR}")
    configure_file(pkgconfig/gtk.pc.in ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/${PCNAME}.pc @ONLY)
    configure_file(pkgconfig/gtk.pc.in ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/gdk-${GTKQT_API}.pc @ONLY)

    set(PC_INCLUDEDIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/gtk-made-qt)
    set(PC_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})
    set(PC_EXTRA_LIBS "")
    configure_file(pkgconfig/gtk.pc.in ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig-install/${PCNAME}.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig-install/${PCNAME}.pc
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/gtk-made-qt/pkgconfig)
endforeach()

# Helper used by examples/tests: build a plain C program against a GTK API level.
function(gtkqt_add_program name source api)
    add_executable(${name} ${source})
    set_source_files_properties(${source} PROPERTIES LANGUAGE C)
    target_compile_definitions(${name} PRIVATE GTKQT_API_VERSION=${api})
    target_link_libraries(${name} PRIVATE gtk-made-qt m)
    # GtkBuilder looks up signal handlers by name in the executable.
    set_target_properties(${name} PROPERTIES ENABLE_EXPORTS ON)
endfunction()

if(GTKQT_BUILD_EXAMPLES)
    foreach(api 2 3 4)
        file(GLOB EXAMPLE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/examples/gtk${api}/*.c)
        foreach(src ${EXAMPLE_SOURCES})
            get_filename_component(n ${src} NAME_WE)
            gtkqt_add_program(gtk${api}-${n} ${src} ${api})
            set_target_properties(gtk${api}-${n} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/examples)
        endforeach()
    endforeach()
endif()

if(GTKQT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    # Smoke-test every example: start it headless, let it run its event loop
    # briefly, then exit (GTKQT_AUTOQUIT is handled inside the library).
    if(GTKQT_BUILD_EXAMPLES)
        foreach(api 2 3 4)
            file(GLOB EXAMPLE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/examples/gtk${api}/*.c)
            foreach(src ${EXAMPLE_SOURCES})
                get_filename_component(n ${src} NAME_WE)
                add_test(NAME example-gtk${api}-${n} COMMAND gtk${api}-${n})
                set_tests_properties(example-gtk${api}-${n} PROPERTIES
                    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;GTKQT_AUTOQUIT=300" TIMEOUT 20)
            endforeach()
        endforeach()
    endif()
endif()
