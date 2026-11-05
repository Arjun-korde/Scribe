add_library(scribe_editor STATIC
    EditorWidget.h
    EditorWidget.cpp
)

target_include_directories(scribe_editor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scribe_editor PUBLIC scribe_core Qt6::Widgets)
