cmake_minimum_required(VERSION 3.10.0)
project(QuickManPath VERSION 0.1.0 LANGUAGES CXX)

# list(APPEND CMAKE_PREFIX_PATH "D:\\cpp_test\\fltk_install")
find_package(FLTK 1.4 CONFIG REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE  
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/path_table.cpp
    ${CMAKE_SOURCE_DIR}/src/win_env_utils.cpp
    ${CMAKE_SOURCE_DIR}/resource/QuickManPath.rc)
target_link_libraries(${PROJECT_NAME} PRIVATE fltk::fltk)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

include(InstallRequiredSystemLibraries)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP;NSIS")
set(CPACK_SOURCE_GENERATOR "ZIP")

# NSIS specific configuration
set(CPACK_NSIS_DISPLAY_NAME "QuickManPath")
set(CPACK_NSIS_PACKAGE_NAME "QuickManPath")
set(CPACK_NSIS_CONTACT "QuickManPath")
set(CPACK_NSIS_HELP_LINK "https://github.com/JonahZeng/QuickManPath")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/JonahZeng/QuickManPath")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MENU_LINKS
    "bin/QuickManPath.exe" "QuickManPath"
)

include(CPack)

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
