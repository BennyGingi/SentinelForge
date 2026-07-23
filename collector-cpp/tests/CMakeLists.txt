# GoogleTest is downloaded and configured automatically during CMake
# configuration. A fresh clone needs no manual install: FetchContent pulls a
# pinned release, so builds stay reproducible and offline-friendly once cached.
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Match the collector's runtime library on MSVC so gtest links cleanly.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(collector_tests
    RuleValidatorTests.cpp
    ConfigurationTests.cpp
    DetectionEngineTests.cpp
    LoggerTests.cpp
    PerformanceProfilerTests.cpp
    JsonExporterTests.cpp
)

# Tests link the production logic library directly and drive the domain
# objects in-process. They deliberately do not touch main.cpp, the console,
# ReportPrinter, or any external service.
target_link_libraries(collector_tests PRIVATE collector_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(collector_tests)
