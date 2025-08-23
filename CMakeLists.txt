cmake_minimum_required(VERSION 3.20.4)

project(xmodel_gen VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(LINUX)
        find_package(ECM 5.86.0 REQUIRED NO_MODULE)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
endif()

# Set path to our custom CMAKE scripts
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")


# Include Qt deployment helper function
include(windeployqt)

include(FetchContent)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.11.0
)

FetchContent_GetProperties(spdlog)
if (NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()

FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json
  GIT_TAG v3.7.3)

FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
  FetchContent_Populate(nlohmann_json)
  add_subdirectory( ${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

#set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network PrintSupport)

configure_file(src/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)
configure_file(res/installer/xmodel_gen.iss.in ${CMAKE_CURRENT_SOURCE_DIR}/res/installer/xmodel_gen.iss)

file( GLOB_RECURSE BASE_SRC src/*cpp src/*h)
file( GLOB_RECURSE BASE_RES res/*ui res/*qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
qt6_wrap_ui(BASE_SRC res/mainwindow.ui)
else()
qt5_wrap_ui(BASE_SRC res/mainwindow.ui)
endif()


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${BASE_SRC}
        ${BASE_RES}
        res/xmodel_gen.qrc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${BASE_SRC}
            ${BASE_RES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(${PROJECT_NAME}
            ${BASE_SRC}
            ${BASE_RES}
            res/xmodel_gen.qrc
        )
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::PrintSupport spdlog::spdlog nlohmann_json::nlohmann_json)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_sources(${PROJECT_NAME} PRIVATE res/resource.rc)

if(APPLE)
   # NOTE: Don't include the path in MACOSX_BUNDLE_ICON_FILE -- this is
   # the property added to Info.plist
   set(MACOSX_BUNDLE_ICON_FILE xmodel_gen)

   # And this part tells CMake where to find and install the file itself
   set(myApp_ICON ${CMAKE_CURRENT_SOURCE_DIR}/res/xmodel_gen.icns)
   set_source_files_properties(${myApp_ICON} PROPERTIES
       MACOSX_PACKAGE_LOCATION "Resources")
   target_sources(${PROJECT_NAME} PRIVATE ${myApp_ICON})
endif()

if(WIN32)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${BASE_SRC})
source_group(res FILES ${BASE_RES})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
 else()   
    if(WIN32)
        windeployqt(${PROJECT_NAME})
    endif()
    if(APPLE)
    message("macdeployqt")
            execute_process(
            COMMAND ${qt_root}/bin/macdeployqt
            ${CMAKE_INSTALL_PREFIX}/build/${PROJECT_NAME}.app
            -dmg
        )
    endif()
endif()

if(APPLE)
    message(STATUS "APPS: ${APPS}")
    message(STATUS "QT_PLUGINS: ${QT_PLUGINS}")
    message(STATUS "DIRS: ${DIRS}")

install(CODE "include(BundleUtilities)
    fixup_bundle(\"${APPS}\" \"${QT_PLUGINS}\" \"${DIRS}\")")
endif()

if(LINUX)
    message(STATUS "APPS: ${APPS}")
    message(STATUS "QT_PLUGINS: ${QT_PLUGINS}")
    message(STATUS "DIRS: ${DIRS}")

    install(TARGETS ${PROJECT_NAME} DESTINATION bin)
    #install(TARGETS ${PROJECT_NAME} ${INSTALL_TARGETS_DEFAULT_ARGS})

        if(ECM_FOUND)
                include(KDEInstallDirs)
        
            install(PROGRAMS org.flatpak.xmodel_gen.desktop  DESTINATION ${KDE_INSTALL_APPDIR})
            install(FILES org.flatpak.xmodel_gen.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
            install(FILES res/org.flatpak.xmodel_gen.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)
        endif()
endif()

