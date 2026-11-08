find_package(GTest CONFIG REQUIRED)

# Pure C++ logic (GoogleTest)
add_executable(scribe_gtests
    SmokeGTest.cpp
    LineEndingTest.cpp
    EncodingTest.cpp
    InMemoryBackendTest.cpp
    DocumentTest.cpp
    CommandRegistryTest.cpp
    LineNumberTest.cpp
    TextSearchTest.cpp
    SettingsManagerTest.cpp
)
target_link_libraries(scribe_gtests PRIVATE scribe_core GTest::gtest GTest::gtest_main)
add_test(NAME scribe_gtests COMMAND scribe_gtests)

# Qt classes and UI behaviour (Qt Test). Each test class has its own main(), so each one is its
# own executable. Usage: scribe_add_qt_test(<name> <source> [<library>...])

# All test executables deploy the same Qt DLLs and plugins into this folder. Run in parallel,
# windeployqt processes collide ("Cannot create file when it already exists"). The deploy step is
# part of the link edge, so a one-job pool for linking tests runs them one at a time. Ninja only.
set_property(GLOBAL APPEND PROPERTY JOB_POOLS scribe_qt_deploy=1)

function(scribe_add_qt_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARGN})
    set_property(TARGET ${name} PROPERTY JOB_POOL_LINK scribe_qt_deploy)
    add_test(NAME ${name} COMMAND ${name})

    # Copy the Qt DLLs next to the test exe so CTest can run it.
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime "$<TARGET_FILE:${name}>"
        COMMENT "Deploying Qt runtime for ${name}"
    )
endfunction()

scribe_add_qt_test(scribe_qttests SmokeQtTest.cpp)
scribe_add_qt_test(scribe_editor_tests EditorWidgetTest.cpp scribe_editor)
scribe_add_qt_test(scribe_window_tests MainWindowTest.cpp scribe_ui)
scribe_add_qt_test(scribe_tabs_tests EditorTabsTest.cpp scribe_editor)
scribe_add_qt_test(scribe_tabcommands_tests TabCommandsTest.cpp scribe_editor)
scribe_add_qt_test(scribe_controller_tests DocumentControllerTest.cpp scribe_ui)
scribe_add_qt_test(scribe_dialogs_tests FileDialogsTest.cpp scribe_ui)
scribe_add_qt_test(scribe_editcommands_tests EditCommandsTest.cpp scribe_editor)
scribe_add_qt_test(scribe_filedrop_tests FileDropFilterTest.cpp scribe_editor)
scribe_add_qt_test(scribe_viewsettings_tests ViewSettingsTest.cpp scribe_editor)
scribe_add_qt_test(scribe_statusbar_tests EditorStatusBarTest.cpp scribe_editor)
scribe_add_qt_test(scribe_viewcommands_tests ViewCommandsTest.cpp scribe_editor)
scribe_add_qt_test(scribe_encoding_commands_tests EncodingCommandsTest.cpp scribe_ui)
scribe_add_qt_test(scribe_search_commands_tests SearchCommandsTest.cpp scribe_editor)
scribe_add_qt_test(scribe_gotoline_dialog_tests GoToLineDialogTest.cpp scribe_ui)
scribe_add_qt_test(scribe_editorsearch_tests EditorSearchTest.cpp scribe_editor)
scribe_add_qt_test(scribe_findbar_tests FindBarTest.cpp scribe_editor)
scribe_add_qt_test(scribe_viewsettings_persistence_tests ViewSettingsPersistenceTest.cpp scribe_editor)
