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

add_executable(collector
    src/main.cpp
    src/Application.cpp
    src/Logger.cpp
    src/Event.cpp
    src/EventParser.cpp
    src/Rule.cpp
    src/RuleParser.cpp
    src/DetectionEngine.cpp
)

target_include_directories(collector PRIVATE include)
target_link_libraries(collector PRIVATE nlohmann_json::nlohmann_json)

# Absolute paths to the sample telemetry and rule files, resolved at configure
# time so the collector can locate them regardless of the working directory
# it is run from. This is a temporary convenience for local development; a
# future issue will replace it with a proper command-line/config input path.
target_compile_definitions(collector PRIVATE
    SAMPLE_LOG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../sample-logs/process_create.json"
    SAMPLE_RULE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../rules/suspicious_powershell.json"
)

if (MSVC)
    target_compile_options(collector PRIVATE /W4 /WX)
else()
    target_compile_options(collector PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()