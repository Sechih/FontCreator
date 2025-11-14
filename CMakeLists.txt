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

# --- Исполняемая цель ---
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
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  )
elseif(WIN32)
  set_target_properties(FontCreator PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# --- Установка (опционально) ---
include(GNUInstallDirs)
install(TARGETS FontCreator
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if (QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(FontCreator)
endif()

# ======================================================================
#                Windows: windeployqt + QtIFW оффлайн-инсталлятор
# ======================================================================
if (WIN32)
  # ---------- Надёжный поиск windeployqt ----------
  # 1) возьмём путь к qmake из импортированной цели Qt::<qmake>
  if (QT_VERSION_MAJOR EQUAL 6)
    get_target_property(_qt_qmake_exe Qt6::qmake IMPORTED_LOCATION)
  else()
    get_target_property(_qt_qmake_exe Qt5::qmake IMPORTED_LOCATION)
  endif()

  if (NOT _qt_qmake_exe)
    message(FATAL_ERROR "Не удалось найти Qt::qmake. Проверь установку Qt (должен быть установлен полный пакет с qmake).")
  endif()

  get_filename_component(_qt_bin_dir "${_qt_qmake_exe}" DIRECTORY)

  find_program(WINDEPLOYQT_EXECUTABLE
               NAMES windeployqt.exe windeployqt
               HINTS "${_qt_bin_dir}"
               NO_DEFAULT_PATH)
  if (NOT WINDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "Не найден windeployqt.exe рядом с qmake: ${_qt_bin_dir}")
  endif()

  # Куда складываем деплой
  set(APP_DEPLOY_DIR "$<TARGET_FILE_DIR:FontCreator>/deploy")

  # Конфигурация для windeployqt (Debug/Release)
  set(WD_CONF_OPT
      $<$<CONFIG:Debug>:--debug>
      $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:--release>
  )

  add_custom_command(TARGET FontCreator POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo "Deploying Qt runtime into ${APP_DEPLOY_DIR}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${APP_DEPLOY_DIR}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              "$<TARGET_FILE:FontCreator>" "${APP_DEPLOY_DIR}/FontCreator.exe"
      COMMAND "${WINDEPLOYQT_EXECUTABLE}"
              ${WD_CONF_OPT}
              --compiler-runtime
              --dir "${APP_DEPLOY_DIR}"
              "${APP_DEPLOY_DIR}/FontCreator.exe"
      COMMENT "Deploying Qt runtime into ${APP_DEPLOY_DIR}"
      VERBATIM
  )

  # ---------- QtIFW: оффлайн-инсталлятор ----------
  # Путь к Qt Installer Framework (можно переопределить: -DIFW_BIN=C:/Qt/Tools/QtInstallerFramework/4.10/bin)
  set(IFW_BIN "C:/Tools/Qt/Tools/QtInstallerFramework/4.10/bin" CACHE PATH "Path to QtIFW bin")
  if (NOT EXISTS "${IFW_BIN}/binarycreator.exe")
    message(FATAL_ERROR "Не найден QtIFW: ${IFW_BIN}/binarycreator.exe")
  endif()

  # Структура инсталлятора в исходниках
  set(INSTALLER_ROOT   "${CMAKE_SOURCE_DIR}/installer")
  set(INSTALLER_CONFIG "${INSTALLER_ROOT}/config/config.xml")

  # !!! ВАЖНО: СВЯЗЫВАЕМ С ТВОЕЙ ПАПКОЙ !!!
  # У тебя в дереве есть installer/packages/com.canfusion.app/meta/package.xml
  # Поэтому используем именно этот пакет:
  set(PKG_ID "com.canfusion.app")

  set(PKG_META_SRC     "${INSTALLER_ROOT}/packages/${PKG_ID}/meta")
  if (NOT EXISTS "${PKG_META_SRC}/package.xml")
    message(FATAL_ERROR "Не найден ${PKG_META_SRC}/package.xml (PKG_ID='${PKG_ID}')")
  endif()

  # Рабочая структура в build/
  set(IFW_WORK          "${CMAKE_BINARY_DIR}/ifw")
  set(IFW_WORK_CONFIG   "${IFW_WORK}/config/config.xml")
  set(IFW_WORK_PACKAGES "${IFW_WORK}/packages")
  set(PKG_DIR_WORK      "${IFW_WORK_PACKAGES}/${PKG_ID}")
  set(PKG_META_WORK     "${PKG_DIR_WORK}/meta")
  set(PKG_DATA_WORK     "${PKG_DIR_WORK}/data")

  # Итоговый установщик (с учётом конфигурации)
  set(INSTALLER_OUT "${CMAKE_BINARY_DIR}/FontCreator-${PROJECT_VERSION}-$<CONFIG>-setup.exe")

  add_custom_target(installer
    DEPENDS FontCreator

    # 1) Подготовить рабочую структуру
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${IFW_WORK}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PKG_META_WORK}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PKG_DATA_WORK}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${IFW_WORK}/config"

    # 2) Копируем meta и config
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PKG_META_SRC}" "${PKG_META_WORK}"
    COMMAND "${CMAKE_COMMAND}" -E copy "${INSTALLER_CONFIG}" "${IFW_WORK_CONFIG}"

    # 3) Полезная нагрузка: весь deploy + сам EXE
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${APP_DEPLOY_DIR}" "${PKG_DATA_WORK}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:FontCreator>" "${PKG_DATA_WORK}/FontCreator.exe"

    # 4) Сборка оффлайн‑инсталлятора
    COMMAND "${IFW_BIN}/binarycreator.exe"
            -c "${IFW_WORK_CONFIG}"
            -p "${IFW_WORK_PACKAGES}"
            "${INSTALLER_OUT}"

    COMMENT "QtIFW: упаковка ${APP_DEPLOY_DIR} → ${INSTALLER_OUT}"
    VERBATIM
  )

  # Диагностика
  message(STATUS "WINDEPLOYQT      = ${WINDEPLOYQT_EXECUTABLE}")
  message(STATUS "IFW_BIN          = ${IFW_BIN}")
  message(STATUS "INSTALLER_CONFIG = ${INSTALLER_CONFIG}")
  message(STATUS "PKG_ID           = ${PKG_ID}")
  message(STATUS "PKG_META_SRC     = ${PKG_META_SRC}")
  message(STATUS "APP_DEPLOY_DIR   = ${APP_DEPLOY_DIR}")
  message(STATUS "INSTALLER_OUT    = ${INSTALLER_OUT}")
endif()
