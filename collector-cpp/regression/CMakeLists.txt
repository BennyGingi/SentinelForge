# Detection regression harness executable (Issue #037). Links collector_core
# directly to drive the real pipeline in-process; Scenario parsing lives here
# rather than in collector_core since the production collector never needs it.
add_executable(regression_runner
    main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/Scenario.cpp
)

target_link_libraries(regression_runner PRIVATE collector_core)

target_compile_definitions(regression_runner PRIVATE
    DEFAULT_RULES_DIR="${CMAKE_SOURCE_DIR}/../rules"
    DEFAULT_SIGMA_RULES_DIR="${CMAKE_SOURCE_DIR}/../sigma-rules"
    DEFAULT_SCENARIOS_DIR="${CMAKE_SOURCE_DIR}/../sample-attacks"
)

if (MSVC)
    target_compile_options(regression_runner PRIVATE /W4 /WX)
else()
    target_compile_options(regression_runner PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
