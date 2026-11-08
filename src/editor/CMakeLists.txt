add_library(scribe_editor STATIC
    EditorWidget.h
    EditorWidget.cpp
    EditorTabs.h
    EditorTabs.cpp
    ViewSettings.h
    ViewSettings.cpp
    ViewSettingsStore.h
    ViewSettingsStore.cpp
    ViewSettingsPersistence.h
    ViewSettingsPersistence.cpp
    FileDropFilter.h
    FileDropFilter.cpp
    TabCommands.h
    TabCommands.cpp
    EditCommands.h
    EditCommands.cpp
    ViewCommands.h
    ViewCommands.cpp
    SearchCommands.h
    SearchCommands.cpp
    EditorSearch.h
    EditorSearch.cpp
    FindBar.h
    FindBar.cpp
    EditorStatusBar.h
    EditorStatusBar.cpp
)

target_include_directories(scribe_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scribe_editor PUBLIC scribe_core Qt6::Widgets)
