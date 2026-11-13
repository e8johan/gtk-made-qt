cmake_minimum_required(VERSION 3.16)
project(gtk-made-qt VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

include(GNUInstallDirs)

set(GTKQT_QT_MAJOR 6 CACHE STRING "Qt major version to build against (6 or 5)")
option(GTKQT_BUILD_EXAMPLES "Build the GTK example programs against the shim" ON)
option(GTKQT_BUILD_TESTS "Build the test suite" ON)

find_package(Qt${GTKQT_QT_MAJOR} REQUIRED COMPONENTS Widgets PrintSupport)
find_package(PkgConfig REQUIRED)
# GLib/GObject/GIO are plain C utility libraries (not GTK). Qt itself already
# runs on the GLib main loop on Linux, so they come along for free.
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0 gobject-2.0 gio-2.0 gmodule-2.0)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo pango pangocairo)

set(GTKQT_SOURCES
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
    src/gtk2.cpp
    src/print.cpp
    src/script.cpp
    src/gtk1.cpp
    src/gtk1widgets.cpp
    src/gdkx11.c
)

# GtkBuilder's table of the library's own GType functions
set(GTKQT_GENERATED ${CMAKE_CURRENT_BINARY_DIR}/generated)
add_custom_command(OUTPUT ${GTKQT_GENERATED}/types.inc
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GTKQT_GENERATED}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_type_table.py ${GTKQT_GENERATED}/types.inc ${GTKQT_SOURCES}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_type_table.py ${GTKQT_SOURCES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# The library is static: programs resolve the GTK API when they are linked
# and nothing is looked up at run time. Internals have hidden visibility;
# the API keeps default visibility (see gdk.h), which ordinary executables
# do not export. Qt, GLib/GObject/GIO, cairo and Pango stay shared.
#
# Programs with plugins (a shared library or dlopen'ed modules that call
# GTK themselves, e.g. XMMS) need one copy of the toolkit for all of them:
# GTKQT_BUILD_SHARED also builds libgtk-made-qt.so, used through
# "gtk-made-qt-env --shared".
# gdk_threads_enter() hands a thread's GTK calls to the GUI thread with
# swapcontext() (see src/drawing.cpp): in glibc, from libucontext on musl.
include(CheckSymbolExists)
check_symbol_exists(swapcontext ucontext.h GTKQT_HAVE_SWAPCONTEXT)
set(GTKQT_UCONTEXT_LIB "")
if(NOT GTKQT_HAVE_SWAPCONTEXT)
    find_library(GTKQT_LIBUCONTEXT ucontext)
    if(GTKQT_LIBUCONTEXT)
        set(CMAKE_REQUIRED_LIBRARIES ${GTKQT_LIBUCONTEXT})
        check_symbol_exists(swapcontext ucontext.h GTKQT_HAVE_LIBUCONTEXT)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if(GTKQT_HAVE_LIBUCONTEXT)
            set(GTKQT_HAVE_SWAPCONTEXT ON)
            set(GTKQT_UCONTEXT_LIB "-lucontext")
        endif()
    endif()
endif()
if(NOT GTKQT_HAVE_SWAPCONTEXT)
    message(STATUS "No swapcontext: GTK calls from other threads under gdk_threads_enter() are not moved to the GUI thread")
endif()

option(GTKQT_BUILD_SHARED "Also build the shared library for programs with plugins" ON)
function(gtkqt_objects name)
    add_library(${name} OBJECT ${GTKQT_SOURCES} ${GTKQT_GENERATED}/types.inc)
    target_compile_definitions(${name} PRIVATE GTKQT_COMPILATION QT_NO_KEYWORDS ${ARGN})
    if(GTKQT_HAVE_SWAPCONTEXT)
        target_compile_definitions(${name} PRIVATE GTKQT_HAVE_SWAPCONTEXT)
    endif()
    target_include_directories(${name} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> PRIVATE ${GTKQT_GENERATED})
    target_link_libraries(${name} PUBLIC PkgConfig::GLIB PkgConfig::CAIRO Qt::Widgets Qt::PrintSupport)
    set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
endfunction()
gtkqt_objects(gtkqt_objects)
add_library(gtk-made-qt STATIC $<TARGET_OBJECTS:gtkqt_objects>)
target_include_directories(gtk-made-qt PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/gtk-made-qt>)
target_link_libraries(gtk-made-qt PUBLIC PkgConfig::GLIB PkgConfig::CAIRO Qt::Widgets Qt::PrintSupport ${GTKQT_UCONTEXT_LIB})
set_target_properties(gtk-made-qt PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
if(GTKQT_BUILD_SHARED)
    gtkqt_objects(gtkqt_objects_shared GTKQT_EXPORT_API)
    add_library(gtk-made-qt-shared SHARED $<TARGET_OBJECTS:gtkqt_objects_shared>)
    target_include_directories(gtk-made-qt-shared PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
    target_link_libraries(gtk-made-qt-shared PUBLIC PkgConfig::GLIB PkgConfig::CAIRO Qt::Widgets Qt::PrintSupport ${GTKQT_UCONTEXT_LIB})
    set_target_properties(gtk-made-qt-shared PROPERTIES OUTPUT_NAME gtk-made-qt VERSION ${PROJECT_VERSION} SOVERSION 0
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
endif()

# Xlib for GTK programs that call it next to GDK (src/x11/xlib.c): a small
# archive of its own, hidden from Qt's use of the system libX11, that
# -lX11 / -lXext resolve to.
add_library(gtkqt-x11 STATIC src/x11/xlib.c)
target_include_directories(gtkqt-x11 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(gtkqt-x11 PRIVATE PkgConfig::GLIB PkgConfig::CAIRO)
set_target_properties(gtkqt-x11 PROPERTIES POSITION_INDEPENDENT_CODE ON C_VISIBILITY_PRESET hidden
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Drop-in names: lib/libgtk-x11-2.0.so, libgtk-3.so ... are GNU ld scripts
# pulling in the archive and Qt, so -lgtk-x11-2.0 works unchanged; the
# pkg-config files (gtk+-2.0, gtk+-3.0, gtk4 ...) in lib/pkgconfig point
# configure scripts at them.
set(GTKQT_INSTALL_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR}/gtk-made-qt)
set(GTKQT_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/gtk-made-qt)
set(GTKQT_LINK_NAMES gtk gdk gtk-x11-2.0 gdk-x11-2.0 gtk-3 gdk-3 gtk-4)
function(gtkqt_link_script file archive qtlibs)
    file(GENERATE OUTPUT ${file} CONTENT
"/* GNU ld script: GTK made Qt (the GTK API on Qt). */
GROUP ( ${archive} )
INPUT ( ${qtlibs} -lstdc++ -lm ${GTKQT_UCONTEXT_LIB} )
")
endfunction()
set(GTKQT_BUILD_QT "$<TARGET_LINKER_FILE:Qt${GTKQT_QT_MAJOR}::PrintSupport> $<TARGET_LINKER_FILE:Qt${GTKQT_QT_MAJOR}::Widgets> $<TARGET_LINKER_FILE:Qt${GTKQT_QT_MAJOR}::Gui> $<TARGET_LINKER_FILE:Qt${GTKQT_QT_MAJOR}::Core>")
set(GTKQT_INSTALL_QT "-lQt${GTKQT_QT_MAJOR}PrintSupport -lQt${GTKQT_QT_MAJOR}Widgets -lQt${GTKQT_QT_MAJOR}Gui -lQt${GTKQT_QT_MAJOR}Core")
set(GTKQT_STAGE ${CMAKE_CURRENT_BINARY_DIR}/install-files)
foreach(n ${GTKQT_LINK_NAMES})
    gtkqt_link_script(${CMAKE_CURRENT_BINARY_DIR}/lib/lib${n}.so ${CMAKE_CURRENT_BINARY_DIR}/lib/libgtk-made-qt.a "${GTKQT_BUILD_QT}")
    gtkqt_link_script(${GTKQT_STAGE}/lib${n}.so ${GTKQT_INSTALL_LIBDIR}/libgtk-made-qt.a "${GTKQT_INSTALL_QT}")
endforeach()
foreach(n X11 Xext)
    gtkqt_link_script(${CMAKE_CURRENT_BINARY_DIR}/lib/lib${n}.so
        "${CMAKE_CURRENT_BINARY_DIR}/lib/libgtkqt-x11.a ${CMAKE_CURRENT_BINARY_DIR}/lib/libgtk-made-qt.a" "${GTKQT_BUILD_QT}")
    gtkqt_link_script(${GTKQT_STAGE}/lib${n}.so
        "${GTKQT_INSTALL_LIBDIR}/libgtkqt-x11.a ${GTKQT_INSTALL_LIBDIR}/libgtk-made-qt.a" "${GTKQT_INSTALL_QT}")
    if(GTKQT_BUILD_SHARED)
        file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib/shared/lib${n}.so CONTENT
"/* GNU ld script: GTK made Qt's Xlib layer, private to each object, on the shared library */
GROUP ( ${CMAKE_CURRENT_BINARY_DIR}/lib/libgtkqt-x11.a )
INPUT ( ${CMAKE_CURRENT_BINARY_DIR}/lib/libgtk-made-qt.so.0 )
")
        file(GENERATE OUTPUT ${GTKQT_STAGE}/shared/lib${n}.so CONTENT
"/* GNU ld script: GTK made Qt's Xlib layer, private to each object, on the shared library */
GROUP ( ${GTKQT_INSTALL_LIBDIR}/libgtkqt-x11.a )
INPUT ( ${CMAKE_INSTALL_FULL_LIBDIR}/libgtk-made-qt.so.0 )
")
    endif()
endforeach()

# pkg-config files: name|api|version|library
set(GTKQT_PC_FILES
    "gtk+|1|1.2.10|gtk" "gdk|1|1.2.10|gdk"
    "gtk+-2.0|2|2.24.33|gtk-x11-2.0" "gtk+-x11-2.0|2|2.24.33|gtk-x11-2.0" "gdk-2.0|2|2.24.33|gdk-x11-2.0" "gdk-x11-2.0|2|2.24.33|gdk-x11-2.0"
    "gtk+-3.0|3|3.24.41|gtk-3" "gtk+-x11-3.0|3|3.24.41|gtk-3" "gdk-3.0|3|3.24.41|gdk-3" "gdk-x11-3.0|3|3.24.41|gdk-3"
    "gtk4|4|4.14.0|gtk-4" "gtk4-x11|4|4.14.0|gtk-4")
foreach(pc ${GTKQT_PC_FILES})
    string(REPLACE "|" ";" f "${pc}")
    list(GET f 0 PCNAME)
    list(GET f 1 GTKQT_API)
    list(GET f 2 GTK_FAKE_VERSION)
    list(GET f 3 PC_LIBNAME)
    set(PC_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set(PC_LIBDIR ${CMAKE_CURRENT_BINARY_DIR}/lib)
    configure_file(pkgconfig/gtk.pc.in ${CMAKE_CURRENT_BINARY_DIR}/lib/pkgconfig/${PCNAME}.pc @ONLY)
    set(PC_INCLUDEDIR ${GTKQT_INSTALL_INCLUDEDIR})
    set(PC_LIBDIR ${GTKQT_INSTALL_LIBDIR})
    configure_file(pkgconfig/gtk.pc.in ${GTKQT_STAGE}/pkgconfig/${PCNAME}.pc @ONLY)
endforeach()
foreach(pcname x11 xext)
    foreach(variant "" "shared/")
        foreach(where build stage)
            if(where STREQUAL build)
                set(inc ${CMAKE_CURRENT_SOURCE_DIR}/include)
                set(lib ${CMAKE_CURRENT_BINARY_DIR}/lib/${variant})
                set(out ${CMAKE_CURRENT_BINARY_DIR}/lib/${variant}pkgconfig/${pcname}.pc)
            else()
                set(inc ${GTKQT_INSTALL_INCLUDEDIR})
                set(lib ${GTKQT_INSTALL_LIBDIR}/${variant})
                set(out ${GTKQT_STAGE}/${variant}pkgconfig/${pcname}.pc)
            endif()
            if(pcname STREQUAL x11)
                set(l X11)
            else()
                set(l Xext)
            endif()
            file(WRITE ${out} "Name: ${pcname}\nDescription: Xlib for GTK programs, on GTK made Qt\nVersion: 1.8.7\nCflags: -I${inc}\nLibs: -L${lib} -l${l}\n")
        endforeach()
    endforeach()
endforeach()

# compatibility: build/pkgconfig as before
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig)
foreach(pc ${GTKQT_PC_FILES})
    string(REPLACE "|" ";" f "${pc}")
    list(GET f 0 PCNAME)
    file(CREATE_LINK ${CMAKE_CURRENT_BINARY_DIR}/lib/pkgconfig/${PCNAME}.pc ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/${PCNAME}.pc SYMBOLIC)
endforeach()

# Shared variant (gtk-made-qt-env --shared): <libdir>/gtk-made-qt/shared has
# the same pkg-config files and gtk-config, and lib<name>.so links to the
# shared library, which itself lives in the normal library directory.
if(GTKQT_BUILD_SHARED)
    foreach(pc ${GTKQT_PC_FILES})
        string(REPLACE "|" ";" f "${pc}")
        list(GET f 0 PCNAME)
        list(GET f 1 GTKQT_API)
        list(GET f 2 GTK_FAKE_VERSION)
        list(GET f 3 PC_LIBNAME)
        set(PC_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
        set(PC_LIBDIR ${CMAKE_CURRENT_BINARY_DIR}/lib/shared)
        configure_file(pkgconfig/gtk.pc.in ${CMAKE_CURRENT_BINARY_DIR}/lib/shared/pkgconfig/${PCNAME}.pc @ONLY)
        set(PC_INCLUDEDIR ${GTKQT_INSTALL_INCLUDEDIR})
        set(PC_LIBDIR ${GTKQT_INSTALL_LIBDIR}/shared)
        configure_file(pkgconfig/gtk.pc.in ${GTKQT_STAGE}/shared/pkgconfig/${PCNAME}.pc @ONLY)
    endforeach()
    foreach(n ${GTKQT_LINK_NAMES})
        file(CREATE_LINK ../libgtk-made-qt.so ${CMAKE_CURRENT_BINARY_DIR}/lib/shared/lib${n}.so SYMBOLIC)
        # relative: <libdir>/gtk-made-qt/shared -> <libdir> (packagers want no absolute links)
        file(CREATE_LINK ../../libgtk-made-qt.so.0 ${GTKQT_STAGE}/shared/lib${n}.so SYMBOLIC)
    endforeach()
    foreach(script gtk-config glib-config)
        set(GTKQT_CONFIG_PCDIR ${CMAKE_CURRENT_BINARY_DIR}/lib/shared/pkgconfig)
        configure_file(tools/${script}.in ${CMAKE_CURRENT_BINARY_DIR}/lib/shared/bin/${script} @ONLY
            FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        set(GTKQT_CONFIG_PCDIR ${GTKQT_INSTALL_LIBDIR}/shared/pkgconfig)
        configure_file(tools/${script}.in ${GTKQT_STAGE}/shared/bin/${script} @ONLY)
    endforeach()
endif()

# GTK 1.2 programs ask gtk-config / glib-config instead of pkg-config
foreach(script gtk-config glib-config)
    set(GTKQT_CONFIG_PCDIR ${CMAKE_CURRENT_BINARY_DIR}/lib/pkgconfig)
    configure_file(tools/${script}.in ${CMAKE_CURRENT_BINARY_DIR}/lib/bin/${script} @ONLY
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    set(GTKQT_CONFIG_PCDIR ${GTKQT_INSTALL_LIBDIR}/pkgconfig)
    configure_file(tools/${script}.in ${GTKQT_STAGE}/bin/${script} @ONLY)
    install(PROGRAMS ${GTKQT_STAGE}/bin/${script} DESTINATION ${GTKQT_INSTALL_LIBDIR}/bin)
endforeach()

set(GTKQT_ENV_LIBDIR ${GTKQT_INSTALL_LIBDIR})
configure_file(tools/gtk-made-qt-env.in ${GTKQT_STAGE}/gtk-made-qt-env @ONLY)
set(GTKQT_ENV_LIBDIR ${CMAKE_CURRENT_BINARY_DIR}/lib)
configure_file(tools/gtk-made-qt-env.in ${CMAKE_CURRENT_BINARY_DIR}/gtk-made-qt-env @ONLY
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(WRITE ${GTKQT_STAGE}/gtk-made-qt.pc
"libdir=${GTKQT_INSTALL_LIBDIR}
includedir=${GTKQT_INSTALL_INCLUDEDIR}

Name: GTK made Qt
Description: The GTK 2/3/4 C API on Qt (use the gtk+-2.0 ... files in \${libdir}/pkgconfig)
Version: ${PROJECT_VERSION}
Cflags: -I\${includedir}
Libs: -L\${libdir}
")

install(TARGETS gtk-made-qt gtkqt-x11 ARCHIVE DESTINATION ${GTKQT_INSTALL_LIBDIR})
foreach(n X11 Xext)
    install(FILES ${GTKQT_STAGE}/lib${n}.so DESTINATION ${GTKQT_INSTALL_LIBDIR})
endforeach()
if(GTKQT_BUILD_SHARED)
    install(TARGETS gtk-made-qt-shared LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} NAMELINK_SKIP)
    install(DIRECTORY ${GTKQT_STAGE}/shared/ DESTINATION ${GTKQT_INSTALL_LIBDIR}/shared
            PATTERN "bin/*" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()
foreach(n ${GTKQT_LINK_NAMES})
    install(FILES ${GTKQT_STAGE}/lib${n}.so DESTINATION ${GTKQT_INSTALL_LIBDIR})
endforeach()
install(DIRECTORY ${GTKQT_STAGE}/pkgconfig DESTINATION ${GTKQT_INSTALL_LIBDIR})
install(FILES ${GTKQT_STAGE}/gtk-made-qt.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install(DIRECTORY include/ DESTINATION ${GTKQT_INSTALL_INCLUDEDIR})
install(PROGRAMS ${GTKQT_STAGE}/gtk-made-qt-env DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES COPYING DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/gtk-made-qt)

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
