# GoogleTest is downloaded and configured automatically during CMake
# configuration, mirroring collector-cpp/tests/CMakeLists.txt.
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Match the GUI's runtime library on MSVC so gtest links cleanly.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Pure-logic units only: no widgets, no QApplication event loop. Each source
# below is compiled directly into the test binary rather than linked against
# sentinelforge_desktop, which owns main() and pulls in the full widget tree.
#
# Headers are listed explicitly alongside their .cpp files (matching
# gui/CMakeLists.txt's SF_GUI_HEADERS/SF_GUI_SOURCES pattern) because
# DetectionModel.h/CorrelationModel.h declare Q_OBJECT classes: AUTOMOC's
# same-directory .cpp/.h pairing convention doesn't fire across the
# src/ vs include/ split this repo uses, so without the header listed as
# its own source the moc step is silently skipped and the link fails on
# missing metaObject/qt_metacast/qt_metacall symbols.
add_executable(gui_tests
    MitreCatalogTests.cpp
    TimestampFormatTests.cpp
    DetectionModelTests.cpp
    CorrelationModelTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/mitre/MitreCatalog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/mitre/MitreCatalog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/models/DetectionModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/DetectionModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/models/CorrelationModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/models/CorrelationModel.cpp
)

target_include_directories(gui_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(gui_tests PRIVATE Qt6::Core GTest::gtest_main)

# gtest_discover_tests() runs the binary at build time to enumerate cases,
# and ctest runs it again per case — both need Qt6Core resolvable next to
# the exe (no windeployqt pass happens for this target, unlike the desktop
# app). Must be registered before gtest_discover_tests() so it runs first.
if (WIN32)
    add_custom_command(TARGET gui_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt6::Core>" "$<TARGET_FILE_DIR:gui_tests>"
        COMMENT "Copying Qt6Core runtime next to gui_tests"
    )
endif()

include(GoogleTest)
gtest_discover_tests(gui_tests)
