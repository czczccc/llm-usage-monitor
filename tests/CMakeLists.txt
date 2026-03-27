qt_add_executable(SnapshotAnalyticsTests
    SnapshotAnalyticsTests.cpp
    ../src/core/Models.cpp
    ../src/core/SnapshotAnalytics.cpp
)

target_include_directories(SnapshotAnalyticsTests PRIVATE ../src)
target_link_libraries(SnapshotAnalyticsTests PRIVATE Qt6::Test)
add_test(NAME SnapshotAnalyticsTests COMMAND SnapshotAnalyticsTests)

qt_add_executable(ProviderParserTests
    ProviderParserTests.cpp
    ../src/core/Models.cpp
    ../src/core/ProviderParsers.cpp
)

target_include_directories(ProviderParserTests PRIVATE ../src)
target_link_libraries(ProviderParserTests PRIVATE Qt6::Test)
add_test(NAME ProviderParserTests COMMAND ProviderParserTests)

qt_add_executable(CredentialStoreTests
    CredentialStoreTests.cpp
    ../src/core/Models.cpp
    ../src/storage/CredentialStore.cpp
)

target_include_directories(CredentialStoreTests PRIVATE ../src)
target_link_libraries(CredentialStoreTests PRIVATE Qt6::Test)
if(WIN32)
    target_compile_definitions(CredentialStoreTests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
add_test(NAME CredentialStoreTests COMMAND CredentialStoreTests)
