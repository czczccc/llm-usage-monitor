cmake_minimum_required(VERSION 3.28)

project(LlmUsageMonitor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

include(CTest)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql Charts)

set(APP_SOURCES
    src/main.cpp
    src/app/MainWindow.cpp
    src/app/OverviewPageWidget.cpp
    src/app/UsagePageWidget.cpp
    src/app/SettingsPageWidget.cpp
    src/app/LogPageWidget.cpp
    src/app/ProviderEditorDialog.cpp
    src/core/Models.cpp
    src/core/HttpJsonClient.cpp
    src/core/ProviderParsers.cpp
    src/core/SnapshotAnalytics.cpp
    src/core/ProviderFactory.cpp
    src/core/MonitorService.cpp
    src/providers/DeepSeekAdapter.cpp
    src/providers/OpenAiAdapter.cpp
    src/providers/AnthropicAdapter.cpp
    src/providers/GeminiAdapter.cpp
    src/storage/SettingsStore.cpp
    src/storage/CredentialStore.cpp
    src/storage/SnapshotRepository.cpp
)

set(APP_HEADERS
    src/app/MainWindow.h
    src/app/OverviewPageWidget.h
    src/app/UsagePageWidget.h
    src/app/SettingsPageWidget.h
    src/app/LogPageWidget.h
    src/app/ProviderEditorDialog.h
    src/core/Models.h
    src/core/IProviderAdapter.h
    src/core/HttpJsonClient.h
    src/core/ProviderParsers.h
    src/core/SnapshotAnalytics.h
    src/core/ProviderFactory.h
    src/core/MonitorService.h
    src/providers/DeepSeekAdapter.h
    src/providers/OpenAiAdapter.h
    src/providers/AnthropicAdapter.h
    src/providers/GeminiAdapter.h
    src/storage/SettingsStore.h
    src/storage/CredentialStore.h
    src/storage/SnapshotRepository.h
)

qt_add_executable(LlmUsageMonitor WIN32
    ${APP_SOURCES}
    ${APP_HEADERS}
)

target_link_libraries(LlmUsageMonitor PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Charts
)

target_include_directories(LlmUsageMonitor PRIVATE src)

if(WIN32)
    target_compile_definitions(LlmUsageMonitor PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(LlmUsageMonitor PRIVATE Advapi32)
endif()

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
