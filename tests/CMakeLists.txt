# Headless tests: run with Qt's offscreen platform.
foreach(api 2 3 4)
    gtkqt_add_program(test-api${api} ${CMAKE_CURRENT_SOURCE_DIR}/test-api.c ${api})
    add_test(NAME api${api} COMMAND test-api${api})
    set_tests_properties(api${api} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endforeach()

# GTK 2 era API (item factories, buffer signals, border windows, GCs, printing)
gtkqt_add_program(test-gtk2 ${CMAKE_CURRENT_SOURCE_DIR}/test-gtk2.c 2)
add_test(NAME gtk2 COMMAND test-gtk2)
set_tests_properties(gtk2 PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# GTK 1.2 API (type system, custom widgets, CList, XPM, fonts)
gtkqt_add_program(test-gtk1 ${CMAKE_CURRENT_SOURCE_DIR}/test-gtk1.c 1)
# GTK 1 code must compile with the errors GCC 14 and clang 16 turned on
# (the C mode stays gnu17: C23 has no unprototyped functions, which GTK 1 used)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test-gtk1 PRIVATE -std=gnu17 -Werror=incompatible-pointer-types -Werror=int-conversion
        -Werror=implicit-function-declaration -Werror=implicit-int)
endif()
add_test(NAME gtk1 COMMAND test-gtk1)
set_tests_properties(gtk1 PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Programs with plugins use the shared library: one toolkit for all modules
if(GTKQT_BUILD_SHARED)
    add_executable(test-plugin ${CMAKE_CURRENT_SOURCE_DIR}/test-plugin.c)
    target_compile_definitions(test-plugin PRIVATE GTKQT_API_VERSION=2)
    target_link_libraries(test-plugin PRIVATE gtk-made-qt-shared)
    add_library(test-plugin-module MODULE ${CMAKE_CURRENT_SOURCE_DIR}/test-plugin-module.c)
    target_compile_definitions(test-plugin-module PRIVATE GTKQT_API_VERSION=2)
    target_include_directories(test-plugin-module PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(test-plugin-module PRIVATE gtk-made-qt-shared)
    add_test(NAME plugin COMMAND test-plugin $<TARGET_FILE:test-plugin-module>)
    set_tests_properties(plugin PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# Xlib next to GDK (libgtkqt-x11.a)
gtkqt_add_program(test-x11 ${CMAKE_CURRENT_SOURCE_DIR}/test-x11.c 2)
target_link_libraries(test-x11 PRIVATE gtkqt-x11)
add_test(NAME x11 COMMAND test-x11)
set_tests_properties(x11 PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# A Glade 2 era program (Asunder): worker threads under the GDK lock,
# entry completion, file chooser button, label widgets
gtkqt_add_program(test-asunder ${CMAKE_CURRENT_SOURCE_DIR}/test-asunder.c 2)
add_test(NAME asunder COMMAND test-asunder)
set_tests_properties(asunder PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
