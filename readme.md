# 



Для deploy необходимо вставить в CmakeList
#  Блоки кода необходимые чтобы cmake --install тянул Qt-DLL
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
# Этот блок тебе нужен только чтобы деплой вызывался как цель CMake (deploy_FontCreator) из Qt Creator или через # cmake --build ... --target deploy_FontCreator
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

# вызвать скрипт deploy.cmd 

# для создания установщика. необходимо создать вот такую структуру каталогов:
FontCreator\
  deploy\           <- создадится автоматически после deploy.cmd 
  installer\
    config\config.xml
    packages\com.fontcreator.app\meta\package.xml
    packages\com.fontcreator.app\data <-сюда кладется содержимое каталога deploy, это сделает скрипт
    make_installer.cmd   
# Вызвать make_installer.cmd, установщик будет создан в корне проекта
