qt_add_executable(frameviewer_unit_tests
    unit/FrameIndexTests.cpp
    ../src/frames/FrameIndex.cpp
    ../src/frames/FrameIndexCache.cpp
    ../src/frames/FramePositionResolver.cpp
)
target_include_directories(frameviewer_unit_tests PRIVATE ../src)
target_link_libraries(frameviewer_unit_tests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME frameviewer_unit_tests COMMAND frameviewer_unit_tests)

qt_add_executable(frameviewer_indexer_tests
    integration/FrameIndexerTests.cpp
    ../src/frames/FrameIndex.cpp
    ../src/frames/FrameIndexCache.cpp
    ../src/frames/FrameIndexer.cpp
)
target_include_directories(frameviewer_indexer_tests PRIVATE ../src)
target_link_libraries(frameviewer_indexer_tests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME frameviewer_indexer_tests COMMAND frameviewer_indexer_tests)
set_tests_properties(frameviewer_indexer_tests PROPERTIES TIMEOUT 45)
