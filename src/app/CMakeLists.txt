add_executable(scribe WIN32
    main.cpp
    MainWindow.h
    MainWindow.cpp
    ../../resources/resources.qrc
    ../../resources/scribe.rc
)

target_compile_definitions(scribe PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")
target_link_libraries(scribe PRIVATE Qt6::Widgets)

# Copy the Qt DLLs and platform plugins next to the exe so it runs from the build folder.
add_custom_command(TARGET scribe POST_BUILD
    COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime "$<TARGET_FILE:scribe>"
    COMMENT "Deploying Qt runtime with windeployqt"
)
