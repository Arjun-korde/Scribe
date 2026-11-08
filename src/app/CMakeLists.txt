# The window lives in a library so tests can link it; the executable only adds main().
add_library(scribe_ui STATIC
    MainWindow.h
    MainWindow.cpp
    FileDialogs.h
    FileDialogs.cpp
    DocumentController.h
    DocumentController.cpp
    FileCommands.h
    FileCommands.cpp
    GoToLineDialog.h
    GoToLineDialog.cpp
    EncodingCommands.h
    EncodingCommands.cpp
    WindowPlacementPersistence.h
    WindowPlacementPersistence.cpp
    RecentFilesTracker.h
    RecentFilesTracker.cpp
    Theme.h
    Theme.cpp
    ThemeManager.h
    ThemeManager.cpp
)

target_include_directories(scribe_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scribe_ui PUBLIC scribe_editor scribe_core Qt6::Widgets)

add_executable(scribe WIN32
    main.cpp
    ../../resources/resources.qrc
    ../../resources/scribe.rc
)

target_compile_definitions(scribe PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")
target_link_libraries(scribe PRIVATE scribe_ui Qt6::Widgets)

# Copy the Qt DLLs and platform plugins next to the exe so it runs from the build folder.
add_custom_command(TARGET scribe POST_BUILD
    COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime "$<TARGET_FILE:scribe>"
    COMMENT "Deploying Qt runtime with windeployqt"
)
