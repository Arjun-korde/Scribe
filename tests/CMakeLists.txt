find_package(GTest CONFIG REQUIRED)

# Pure C++ logic (GoogleTest)
add_executable(scribe_gtests SmokeGTest.cpp LineEndingTest.cpp EncodingTest.cpp InMemoryBackendTest.cpp)
target_link_libraries(scribe_gtests PRIVATE scribe_core GTest::gtest GTest::gtest_main)
add_test(NAME scribe_gtests COMMAND scribe_gtests)

# Qt classes and UI behaviour (Qt Test)
add_executable(scribe_qttests SmokeQtTest.cpp)
target_link_libraries(scribe_qttests PRIVATE Qt6::Test)
add_test(NAME scribe_qttests COMMAND scribe_qttests)

# Copy the Qt DLLs next to the Qt Test exe so CTest can run it.
add_custom_command(TARGET scribe_qttests POST_BUILD
    COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime "$<TARGET_FILE:scribe_qttests>"
    COMMENT "Deploying Qt runtime for scribe_qttests"
)
