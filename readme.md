# FontCreator — моё дополнение к TheDotFactory

Небольшая утилита для редактирования глифов (шрифтовых битмапов), сделанная на Qt 6 + MinGW.

Этот файл — памятка **как собрать**, **сделать deploy** и **собрать установщик (installer)**.

---

## 0. Зависимости

- **Qt 6.10.0 (mingw_64)**  
  Пример пути: `C:\Tools\Qt\6.10.0\mingw_64`
- **CMake**  
  Пример: `C:\ST\STM32CubeCLT_1.15.0\CMake\bin\cmake.exe`
- **Qt Installer Framework (QtIFW)**  
  Пример: `C:\Tools\Qt\Tools\QtInstallerFramework\4.10\bin\binarycreator.exe`
- Сборка проекта: kit в Qt Creator  
  `Desktop Qt 6.10.0 MinGW 64-bit`, конфигурация **Release**

---

## 1. CMake: включить установку и автодеплой Qt

В `CMakeLists.txt` должен быть добавлен блок, позволяющий использовать `cmake --install` для
создания портативной сборки (deploy) с Qt DLL.

```cmake
# --- Install tree (where cmake --install will copy files) ---
include(GNUInstallDirs)

install(TARGETS FontCreator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# --- Qt deploy on cmake --install ---
if (QT_VERSION_MAJOR EQUAL 6)
    # Qt 6 generates a script that internally calls windeployqt
    qt_generate_deploy_app_script(
        TARGET FontCreator
        OUTPUT_SCRIPT fc_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        # Optional: extra options for windeployqt:
        # DEPLOY_TOOL_OPTIONS --no-compiler-runtime
    )
    install(SCRIPT ${fc_deploy_script})

    qt_finalize_executable(FontCreator)
endif()
```

---

## 2. Удобная CMake-цель `deploy_FontCreator`

Чтобы можно было запускать деплой прямо из Qt Creator или командой `cmake --build ... --target deploy_FontCreator`,
в `CMakeLists.txt` добавлен кастомный таргет:

```cmake
# ----------------------------------------------------------------------
# Custom deploy target: deploy_<project_name>
# Example: deploy_FontCreator
#
# Allows calling deploy from Qt Creator (target "deploy_FontCreator")
# or from command line:
#   cmake --build <build_dir> --target deploy_FontCreator
# ----------------------------------------------------------------------
if (WIN32 AND QT_VERSION_MAJOR EQUAL 6)
    # Where to put the deployed application (default: ./deploy in project root)
    set(APP_DEPLOY_PREFIX "${CMAKE_SOURCE_DIR}/deploy"
        CACHE PATH "Install / deploy prefix")

    add_custom_target(deploy_${PROJECT_NAME}
        COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}"
                --prefix "${APP_DEPLOY_PREFIX}"
        COMMENT "Deploying ${PROJECT_NAME} to ${APP_DEPLOY_PREFIX}"
    )

    # Make sure the executable is built before deploy
    add_dependencies(deploy_${PROJECT_NAME} ${PROJECT_NAME})
endif()
```

---

## 3. Как сделать deploy (портативная сборка)

### Вариант A — через Qt Creator

1. Выбрать kit: **Desktop Qt 6.10.0 MinGW 64-bit**
2. Конфигурация: **Release**
3. В целях сборки выбрать таргет: `deploy_FontCreator`
4. Нажать **Build**

### Вариант B — через командную строку

Пример для MinGW Makefiles, Release:

```bat
cd C:\Users\<User>\Documents\MY_WORK\FontCreator\build\Desktop_Qt_6_10_0_MinGW_64_bit-Release
cmake --build . --target deploy_FontCreator
```

После успешного деплоя структура в корне проекта:

```text
FontCreator\
  deploy\
    bin\
      FontCreator.exe
      Qt6Core.dll
      Qt6Gui.dll
      Qt6Widgets.dll
      Qt6Network.dll
      Qt6Svg.dll
      libgcc_s_seh-1.dll
      libstdc++-6.dll
      libwinpthread-1.dll
      opengl32sw.dll
      qt.conf
    plugins\
      platforms\qwindows.dll
      imageformats\*.dll
      styles\*.dll
      ...
    translations\
      qt_ru.qm
      и др. qt_*.qm
```

`deploy\bin\FontCreator.exe` уже можно запускать на чистой Windows — это портативная сборка.

---

## 4. Структура для установщика (installer)

Для создания `setup.exe` используется Qt Installer Framework.  
Ожидаемая структура в корне проекта:

```text
FontCreator\
  deploy\                          <- создаётся автоматически после deploy
  installer\
    config\
      config.xml
    packages\
      com.fontcreator.app\
        meta\
          package.xml
        data\                      <- сюда скопируется содержимое deploy (автоматически скриптом)
    make_installer.cmd             <- скрипт сборки инсталлятора
```

### Важные файлы

- `installer/config/config.xml`  
  Общая конфигурация инсталлятора (имя, версия, TargetDir и т.д.)
- `installer/packages/com.fontcreator.app/meta/package.xml`  
  Метаданные пакета: DisplayName, Version, описание и т.п.
- `installer/packages/com.fontcreator.app/data`  
  Содержимое этого каталога — то, что инсталлятор установит пользователю.  
  **Заполняется автоматически** из каталога `deploy` с помощью `make_installer.cmd`.

---

## 5. Скрипт `make_installer.cmd`

Файл:  
`FontCreator\installer\make_installer.cmd`

Скрипт делает три шага:

1. Запускает `cmake --install` для обновления `deploy`.
2. Очищает и пересоздаёт `installer/packages/com.fontcreator.app/data` на основе содержимого `deploy`.
3. Вызывает `binarycreator.exe` (Qt IFW) и создаёт `FontCreator-1.0.0-setup.exe` в корне проекта.

Пути внутри скрипта настроены под:

- CMake:  
  `C:\ST\STM32CubeCLT_1.15.0\CMake\bin\cmake.exe`
- QtIFW (binarycreator):  
  `C:\Tools\Qt\Tools\QtInstallerFramework\4.10\bin\binarycreator.exe`
- Build-директорию Release:  
  `build\Desktop_Qt_6_10_0_MinGW_64_bit-Release`

При необходимости можно править эти пути в самом `make_installer.cmd`.

---

## 6. Как собрать инсталлятор шаг за шагом

1. **Собрать проект в Release** через Qt Creator (kit: `Desktop Qt 6.10.0 MinGW 64-bit`).
2. Убедиться, что Qt Installer Framework установлен, и файл:
   ```text
   C:\Tools\Qt\Tools\QtInstallerFramework\4.10\bin\binarycreator.exe
   ```
   существует (или поправить путь в `make_installer.cmd`).
3. Убедиться, что структуры `installer/config/config.xml` и  
   `installer/packages/com.fontcreator.app/meta/package.xml` на месте.
4. Запустить:

   ```text
   FontCreator\installer\make_installer.cmd
   ```

5. После успешного выполнения скрипта в корне проекта появится:

   ```text
   FontCreator-1.0.0-setup.exe
   ```

Это готовый оффлайн-инсталлятор FontCreator, который можно запускать на любой машине.

---

## 7. Быстрая шпаргалка

- **Deploy (портативная сборка):**
  - В Qt Creator: цель `deploy_FontCreator`, сборка Release.
  - Или из консоли:

    ```bat
    cd build\Desktop_Qt_6_10_0_MinGW_64_bit-Release
    cmake --build . --target deploy_FontCreator
    ```

- **Installer (setup.exe):**

  ```bat
  cd FontCreator\installer
  make_installer.cmd
  ```

Результат: `FontCreator-1.0.0-setup.exe` в корне проекта.
