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
    src/Configuration.cpp
    src/Logger.cpp
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

target_include_directories(collector PRIVATE include)
target_link_libraries(collector PRIVATE nlohmann_json::nlohmann_json)

# The collector reads all runtime settings from the configuration subsystem
# rather than from hardcoded paths. Only the location of the config file, and
# the fallback defaults used when it is absent, are baked in here. They are
# resolved to absolute paths at configure time so the collector behaves the
# same regardless of the working directory it is launched from.
target_compile_definitions(collector PRIVATE
    CONFIG_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../config/sentinelforge.json"
    DEFAULT_RULES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../rules"
    DEFAULT_SAMPLE_EVENT_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../sample-logs/process_create.json"
    DEFAULT_OUTPUT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../output"
)

if (MSVC)
    target_compile_options(collector PRIVATE /W4 /WX)
else()
    target_compile_options(collector PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()