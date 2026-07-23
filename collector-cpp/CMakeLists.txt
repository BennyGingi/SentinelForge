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

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
# yaml-cpp 0.8.0 still declares cmake_minimum_required(< 3.5); allow configure
# on newer CMake without forking the dependency.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_Declare(
    yaml-cpp
    URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(yaml-cpp)

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
    src/NormalizedEvent.cpp
    src/EventNormalizer.cpp
    src/Rule.cpp
    src/RuleParser.cpp
    src/RuleValidator.cpp
    src/RuleLoader.cpp
    src/SigmaRule.cpp
    src/SigmaParser.cpp
    src/SigmaTranslator.cpp
    src/SigmaLoader.cpp
    src/EventMonitor.cpp
    src/DetectionEngine.cpp
    src/DetectionReport.cpp
    src/ReportPrinter.cpp
)

target_include_directories(collector_core PUBLIC include)
target_link_libraries(collector_core PUBLIC
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
)

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
    DEFAULT_SIGMA_RULES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../sigma-rules"
    DEFAULT_MONITOR_INPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../events/incoming"
    DEFAULT_MONITOR_PROCESSED_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../events/processed"
    DEFAULT_MONITOR_FAILED_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../events/failed"
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
