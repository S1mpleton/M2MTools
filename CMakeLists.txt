cmake_minimum_required(VERSION 3.19)
project(M2MTools VERSION 0.2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Sql LinguistTools)
find_package(OpenSSL 1.1.1 REQUIRED COMPONENTS Crypto)

add_library(M2MCore STATIC
    core/reactionvariant.cpp core/variantimage.cpp core/variantarchive.cpp core/result.cpp core/errorcode.cpp
    data/database.cpp data/schemamanager.cpp data/transaction.cpp
    data/variantrepository.cpp data/variantservice.cpp
    infrastructure/logger.cpp)
target_include_directories(M2MCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(M2MCore PUBLIC Qt6::Core Qt6::Gui Qt6::Sql OpenSSL::Crypto)

add_library(M2MUi STATIC
    ui/mainwindow.cpp ui/mainwindow.ui ui/appicons.cpp ui/widgets/reactiontable.cpp ui/widgets/wrappinglabel.cpp
    ui/widgets/variantimagepanel.cpp ui/widgets/variantimagepanel.ui
    ui/delegates/reactiondelegate.cpp validation/reactionvalidator.cpp
    ui/variantslist.cpp ui/variantslist.ui ui/logwindow.cpp
    ui/dialogs/resultmessenger.cpp ui/translateresult.cpp
    infrastructure/translationmanager.cpp)
target_link_libraries(M2MUi PUBLIC M2MCore Qt6::Widgets)

qt_add_executable(M2MTools WIN32 MACOSX_BUNDLE main.cpp)
target_link_libraries(M2MTools PRIVATE M2MUi)
set(TRANSLATION_TARGETS M2MTools)
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(M2MTests tests/lite_tests.cpp)
    target_link_libraries(M2MTests PRIVATE M2MUi Qt6::Test)
    list(APPEND TRANSLATION_TARGETS M2MTests)
    add_test(NAME M2M COMMAND M2MTests)
    set_tests_properties(M2M PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
qt_add_translations(TARGETS ${TRANSLATION_TARGETS} TS_FILES M2MTools_ru_RU.ts)
include(GNUInstallDirs)
install(TARGETS M2MTools BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
