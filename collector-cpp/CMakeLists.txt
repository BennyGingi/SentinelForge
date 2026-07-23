cmake_minimum_required(VERSION 3.20)

project(SentinelCollector CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# All collector logic lives in a static library so it can be linked by both the
# production executable and the unit-test executable without recompiling or
# duplicating the source list. main.cpp is intentionally excluded: it belongs
# only to the executable, never to the tests.
add_library(collector_core STATIC
    src/Application.cpp
    src/Configuration.cpp
    src/Logger.cpp
    src/PerformanceProfiler.cpp
    src/JsonExporter.cpp
    src/Event.cpp
    src/EventParser.cpp
    src/Rule.cpp
    src/RuleParser.cpp
    src/RuleValidator.cpp
    src/RuleLoader.cpp
    src/DetectionEngine.cpp
    src/DetectionReport.cpp
    src/ReportPrinter.cpp
)

target_include_directories(collector_core PUBLIC include)
target_link_libraries(collector_core PUBLIC nlohmann_json::nlohmann_json)

# The collector reads all runtime settings from the configuration subsystem
# rather than from hardcoded paths. Only the location of the config file, and
# the fallback defaults used when it is absent, are baked in here. They are
# resolved to absolute paths at configure time so the collector behaves the
# same regardless of the working directory it is launched from.
target_compile_definitions(collector_core PRIVATE
    CONFIG_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../config/sentinelforge.json"
    DEFAULT_RULES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../rules"
    DEFAULT_SAMPLE_EVENT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../sample-logs/process_create.json"
    DEFAULT_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../output"
    DEFAULT_LOG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../logs/sentinelforge.log"
    DEFAULT_JSON_EXPORT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../detections.json"
)

add_executable(collector src/main.cpp)
target_link_libraries(collector PRIVATE collector_core)

if (MSVC)
    target_compile_options(collector_core PRIVATE /W4 /WX)
    target_compile_options(collector PRIVATE /W4 /WX)
else()
    target_compile_options(collector_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(collector PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Unit testing (GoogleTest) is fetched and configured in the tests subdirectory.
enable_testing()
add_subdirectory(tests)
