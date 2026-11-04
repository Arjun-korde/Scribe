add_library(scribe_core STATIC
    LineEnding.h
    LineEnding.cpp
    Encoding.h
    Encoding.cpp
)

target_include_directories(scribe_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scribe_core PUBLIC Qt6::Core)
