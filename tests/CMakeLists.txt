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
add_test(NAME gtk1 COMMAND test-gtk1)
set_tests_properties(gtk1 PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
