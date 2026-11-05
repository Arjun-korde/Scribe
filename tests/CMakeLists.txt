find_package(GTest CONFIG REQUIRED)

# Pure C++ logic (GoogleTest)
add_executable(scribe_gtests
    SmokeGTest.cpp
    LineEndingTest.cpp
    EncodingTest.cpp
    InMemoryBackendTest.cpp
    DocumentTest.cpp
    CommandRegistryTest.cpp
)
target_link_libraries(scribe_gtests PRIVATE scribe_core GTest::gtest GTest::gtest_main)
add_test(NAME scribe_gtests COMMAND scribe_gtests)

# Qt classes and UI behaviour (Qt Test). Each test class has its own main(), so each one is its
# own executable. Usage: scribe_add_qt_test(<name> <source> [<library>...])
function(scribe_add_qt_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARGN})
    add_test(NAME ${name} COMMAND ${name})

    # Copy the Qt DLLs next to the test exe so CTest can run it.
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime "$<TARGET_FILE:${name}>"
        COMMENT "Deploying Qt runtime for ${name}"
    )
endfunction()

scribe_add_qt_test(scribe_qttests SmokeQtTest.cpp)
scribe_add_qt_test(scribe_editor_tests EditorWidgetTest.cpp scribe_editor)
