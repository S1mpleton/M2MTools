cmake_minimum_required(VERSION 3.16)
project(M2MToolsEnvironmentSmoke LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)
find_package(OpenSSL 1.1.1 REQUIRED COMPONENTS Crypto)
# Reuse the real application objects; replace only its interactive main().
file(GLOB_RECURSE app_objects
    "/workspace/.cloud-setup/build/CMakeFiles/M2MTools.dir/*.o")
list(FILTER app_objects EXCLUDE REGEX "/main\\.cpp\\.o$")
if(NOT app_objects)
    message(FATAL_ERROR "Build M2MTools before the environment smoke check")
endif()
add_executable(environment-smoke smoke.cpp ${app_objects})
target_include_directories(environment-smoke PRIVATE
    /workspace/M2MTools /workspace/.cloud-setup/include)
target_link_libraries(environment-smoke PRIVATE
    Qt6::Widgets Qt6::Sql OpenSSL::Crypto)
enable_testing()
add_test(NAME environment-smoke COMMAND environment-smoke)
set_tests_properties(environment-smoke PROPERTIES TIMEOUT 20)
