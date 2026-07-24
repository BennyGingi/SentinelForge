cmake_minimum_required(VERSION 3.21)

project(SentinelForgeDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Svg)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/theme.cmake)

set(SF_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(SF_ICON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons)
set(SF_MITRE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/mitre)
set(SF_BRANDING_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/branding)
file(MAKE_DIRECTORY ${SF_GENERATED_DIR})
file(MAKE_DIRECTORY ${SF_GENERATED_DIR}/styles)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Theme.h.in
    ${SF_GENERATED_DIR}/Theme.h
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/styles/dark.qss.in
    ${SF_GENERATED_DIR}/styles/dark.qss
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/sentinelforge.qrc.in
    ${SF_GENERATED_DIR}/sentinelforge.qrc
    @ONLY
)

# Git commit hash baked in at configure time (About dialog build identifier).
# Re-run cmake to refresh it after committing new changes.
find_package(Git QUIET)
set(SF_GIT_HASH "unknown")
if (Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE SF_GIT_HASH_OUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE SF_GIT_HASH_RESULT
    )
    if (SF_GIT_HASH_RESULT EQUAL 0 AND SF_GIT_HASH_OUT)
        set(SF_GIT_HASH ${SF_GIT_HASH_OUT})
    endif()
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/AppInfo.h.in
    ${SF_GENERATED_DIR}/AppInfo.h
    @ONLY
)

set(SF_GUI_HEADERS
    include/MainWindow.h
    include/telemetry/ITelemetrySource.h
    include/telemetry/MockTelemetrySource.h
    include/models/DetectionModel.h
    include/models/CorrelationModel.h
    include/models/DetectionFilterProxy.h
    include/ops/LiveOpsController.h
    include/ops/ThreatScorer.h
    include/nav/NavigationController.h
    include/pages/DashboardPage.h
    include/mitre/MitreCatalog.h
    include/widgets/Panel.h
    include/widgets/CollapsibleSection.h
    include/widgets/SeverityRailDelegate.h
    include/widgets/CorrelationDelegate.h
    include/widgets/DetectionTableView.h
    include/widgets/CorrelationTableView.h
    include/widgets/StatusStrip.h
    include/widgets/SparklineWidget.h
    include/widgets/PerformancePanel.h
    include/widgets/LogViewer.h
    include/widgets/InspectorPane.h
    include/widgets/RuleDetailDialog.h
    include/widgets/LiveControlsBar.h
    include/widgets/CriticalToast.h
    include/widgets/AboutDialog.h
)

set(SF_GUI_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/telemetry/MockTelemetrySource.cpp
    src/models/DetectionModel.cpp
    src/models/CorrelationModel.cpp
    src/models/DetectionFilterProxy.cpp
    src/ops/LiveOpsController.cpp
    src/ops/ThreatScorer.cpp
    src/nav/NavigationController.cpp
    src/pages/DashboardPage.cpp
    src/mitre/MitreCatalog.cpp
    src/widgets/Panel.cpp
    src/widgets/CollapsibleSection.cpp
    src/widgets/SeverityRailDelegate.cpp
    src/widgets/CorrelationDelegate.cpp
    src/widgets/DetectionTableView.cpp
    src/widgets/CorrelationTableView.cpp
    src/widgets/StatusStrip.cpp
    src/widgets/SparklineWidget.cpp
    src/widgets/PerformancePanel.cpp
    src/widgets/LogViewer.cpp
    src/widgets/InspectorPane.cpp
    src/widgets/RuleDetailDialog.cpp
    src/widgets/LiveControlsBar.cpp
    src/widgets/CriticalToast.cpp
    src/widgets/AboutDialog.cpp
)

qt_add_executable(sentinelforge_desktop
    ${SF_GUI_HEADERS}
    ${SF_GUI_SOURCES}
    ${SF_GENERATED_DIR}/sentinelforge.qrc
)

target_include_directories(sentinelforge_desktop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SF_GENERATED_DIR}
)

if (WIN32)
    target_sources(sentinelforge_desktop PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/branding/app.rc
    )
endif()

target_link_libraries(sentinelforge_desktop PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)

if (MSVC)
    target_compile_options(sentinelforge_desktop PRIVATE /W4 /permissive-)
else()
    target_compile_options(sentinelforge_desktop PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(sentinelforge_desktop PROPERTIES
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)

if (WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS "${Qt6_DIR}/../../../bin" "${CMAKE_PREFIX_PATH}/bin"
    )
    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET sentinelforge_desktop POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime
                    --include-plugins qsvg,qsvgicon
                    \"$<TARGET_FILE:sentinelforge_desktop>\"
            COMMENT "Deploying Qt runtime with windeployqt"
        )
    endif()
endif()

# Unit testing (GoogleTest) for pure-logic units only — no widgets, no
# QApplication event loop. See gui/tests/CMakeLists.txt.
enable_testing()
add_subdirectory(tests)
