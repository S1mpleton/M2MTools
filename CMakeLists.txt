cmake_minimum_required(VERSION 3.16)

project(M2MTools VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Sql)


set(TS_FILES M2MTools_ru_RU.ts)


set(PROJECT_SOURCES
        main.cpp
        ui/mainwindow.cpp
        ui/mainwindow.h
        ui/mainwindow.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(M2MTools
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        core/automatondata.h core/automatondata.cpp
        core/namelistparser.h core/namelistparser.cpp


        core/hash/hashprovider.h
        core/hash/saltedsha256provider.h core/hash/saltedsha256provider.cpp
        core/hash/rsaprovider.h core/hash/rsaprovider.cpp

        validation/editvalidator.h validation/editvalidator.cpp
        validation/listnamesvalidator.h validation/listnamesvalidator.cpp
        validation/compositevalidator.h validation/compositevalidator.cpp

        ui/delegates/mooredelegate.h ui/delegates/mooredelegate.cpp
        ui/delegates/mealydelegate.h ui/delegates/mealydelegate.cpp

        ui/presenters/fielderrorhighlighter.h ui/presenters/fielderrorhighlighter.cpp
        ui/presenters/tablecellerrorpresenter.h ui/presenters/tablecellerrorpresenter.cpp
        ui/presenters/ierrorpresenter.h
        ui/presenters/errorpresenterfactory.h ui/presenters/errorpresenterfactory.cpp

        resources.qrc
        data/sqlrequests.h
        data/database.h data/database.cpp
        data/schemamanager.h data/schemamanager.cpp
        data/transaction.h data/transaction.cpp
        data/variantrepository.h data/variantrepository.cpp
        data/variantservice.h data/variantservice.cpp
        core/result.h
        infrastructure/logger.h infrastructure/logger.cpp
        core/result.cpp
        infrastructure/translationmanager.h infrastructure/translationmanager.cpp
        core/errorcode.h
        ui/translateresult.h ui/translateresult.cpp
        core/errorcode.cpp
        data/sqlerrormapper.h data/sqlerrormapper.cpp
    )

    target_include_directories(M2MTools PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET M2MTools APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation


    qt_add_translations(M2MTools
        TS_FILES M2MTools_ru_RU.ts
    )
    qt_add_lupdate(M2MTools)


#    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(M2MTools SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(M2MTools
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(M2MTools PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.M2MTools)
endif()
set_target_properties(M2MTools PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS M2MTools
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(M2MTools)
endif()
