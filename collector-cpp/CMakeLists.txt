cmake_minimum_required(VERSION 3.20)

project(SentinelCollector VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
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
    URL_HASH SHA256=fbe74bbdcee21d656715688706da3c8becfd946d92cd44705cc6098bb23b3a16
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(yaml-cpp)

# Header-only, MIT. No TLS/zlib/brotli needed -- the API binds 127.0.0.1
# only (see Configuration::ApiBindAddress), so skip OpenSSL entirely rather
# than add a dependency this project has no use for.
set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_REQUIRE_ZLIB OFF CACHE BOOL "" FORCE)
set(HTTPLIB_REQUIRE_BROTLI OFF CACHE BOOL "" FORCE)
set(HTTPLIB_COMPILE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.15.3.tar.gz
    URL_HASH SHA256=2121bbf38871bb2aafb5f7f2b9b94705366170909f434428352187cb0216124e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(httplib)

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
    src/CorrelationAlert.cpp
    src/CorrelationEngine.cpp
    src/OfficeLaunchesPowerShellRule.cpp
    src/DetectionReport.cpp
    src/ReportPrinter.cpp
    src/TelemetryStore.cpp
    src/ApiServer.cpp
)

target_include_directories(collector_core PUBLIC include)
target_link_libraries(collector_core PUBLIC
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    httplib::httplib
)

# The collector reads all runtime settings from the configuration subsystem
# rather than from hardcoded paths. Only the location of the config file, and
# the fallback defaults used when it is absent, are baked in here. They are
# resolved to absolute paths at configure time so the collector behaves the
# same regardless of the working directory it is launched from.
#
# REPO_ROOT is computed with get_filename_component(... DIRECTORY) rather than
# appending a literal "/.." -- that mode is pure lexical string manipulation
# (no filesystem access), whereas a baked-in ".." component makes every path
# below only resolvable if the collector-cpp/ directory itself still exists
# next to it at runtime. It doesn't in the Docker runtime image, which ships
# only the compiled binary.
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
target_compile_definitions(collector_core PRIVATE
    CONFIG_FILE_PATH="${REPO_ROOT}/config/sentinelforge.json"
    DEFAULT_RULES_DIR="${REPO_ROOT}/rules"
    DEFAULT_SAMPLE_EVENT_FILE="${REPO_ROOT}/sample-logs/process_create.json"
    DEFAULT_OUTPUT_DIR="${REPO_ROOT}/output"
    DEFAULT_LOG_PATH="${REPO_ROOT}/logs/sentinelforge.log"
    DEFAULT_JSON_EXPORT_FILE="${REPO_ROOT}/detections.json"
    DEFAULT_SIGMA_RULES_DIR="${REPO_ROOT}/sigma-rules"
    DEFAULT_MONITOR_INPUT_DIR="${REPO_ROOT}/events/incoming"
    DEFAULT_MONITOR_PROCESSED_DIR="${REPO_ROOT}/events/processed"
    DEFAULT_MONITOR_FAILED_DIR="${REPO_ROOT}/events/failed"
    COLLECTOR_VERSION="${PROJECT_VERSION}"
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

# Detection regression harness (Issue #037).
add_subdirectory(regression)
