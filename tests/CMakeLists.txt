# Headless tests: run with Qt's offscreen platform.
foreach(api 2 3 4)
    gtkqt_add_program(test-api${api} ${CMAKE_CURRENT_SOURCE_DIR}/test-api.c ${api})
    add_test(NAME api${api} COMMAND test-api${api})
    set_tests_properties(api${api} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endforeach()
