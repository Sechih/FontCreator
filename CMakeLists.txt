cmake_minimum_required(VERSION 3.16)
project(FontCreator VERSION 0.1 LANGUAGES CXX)

# --- Qt автоген ---
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Qt ---
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# --- Источники ---
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    pixelgridwidget.cpp
    pixelgridwidget.h
)

# --- Исполняемый файл ---
if (QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(FontCreator
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(FontCreator ${PROJECT_SOURCES})
endif()

target_include_directories(FontCreator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(FontCreator PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# --- Свойства приложения ---
if(APPLE)
  set_target_properties(FontCreator PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
      MACOSX_BUNDLE_SHORT_VERSION_STRING
          ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
elseif(WIN32)
  set_target_properties(FontCreator PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# --- Установка (install-дерево) ---
include(GNUInstallDirs)

install(TARGETS FontCreator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# --- Qt-деплой при cmake --install ---
if (QT_VERSION_MAJOR EQUAL 6)
    # Qt 6 сам сгенерирует скрипт, который внутри вызовет windeployqt
    qt_generate_deploy_app_script(
        TARGET FontCreator
        OUTPUT_SCRIPT fc_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        # сюда при желании можно добавить опции для windeployqt:
        # DEPLOY_TOOL_OPTIONS --no-compiler-runtime
    )
    install(SCRIPT ${fc_deploy_script})

    qt_finalize_executable(FontCreator)
endif()

# ----------------------------------------------------------------------
# Удобная цель для деплоя: deploy_<имя проекта>
# Пример: deploy_FontCreator
# ----------------------------------------------------------------------
if (WIN32 AND QT_VERSION_MAJOR EQUAL 6)
    # Куда складывать готовое приложение (по умолчанию ./deploy в корне проекта)
    set(APP_DEPLOY_PREFIX "${CMAKE_SOURCE_DIR}/deploy"
        CACHE PATH "Install / deploy prefix")

    # Создаём кастомную цель: deploy_FontCreator
    add_custom_target(deploy_${PROJECT_NAME}
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
                --prefix "${APP_DEPLOY_PREFIX}"
        COMMENT "Deploying ${PROJECT_NAME} to ${APP_DEPLOY_PREFIX}"
    )

    # Перед деплоем обязательно собрать exe
    add_dependencies(deploy_${PROJECT_NAME} ${PROJECT_NAME})
endif()
