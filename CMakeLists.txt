cmake_minimum_required(VERSION 3.16)

project(FontCreator VERSION 0.1 LANGUAGES CXX)

# Автоматическая генерация MOC/UIC/RCC
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Источники проекта
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    pixelgridwidget.cpp
    pixelgridwidget.h
)

# Создаём цель (важно: до использования target_*)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(FontCreator
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(FontCreator ${PROJECT_SOURCES})
endif()

# Пути к заголовкам должны идти ПОСЛЕ создания цели
target_include_directories(FontCreator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}  # чтобы ui_mainwindow.h видел pixelgridwidget.h
    ${CMAKE_CURRENT_BINARY_DIR}  # на всякий случай для autogen/includes
)

# Линки
target_link_libraries(FontCreator PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Параметры бандла/Win32 (опционально)
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.FontCreator)
endif()
set_target_properties(FontCreator PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Установка (опционально)
include(GNUInstallDirs)
install(TARGETS FontCreator
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Финализация (Qt 6)
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(FontCreator)
endif()
