cmake_minimum_required(VERSION 3.21)
project(OrbisPDF VERSION 6.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 Pdf)
qt_standard_project_setup(REQUIRES 6.11)

qt_add_executable(OrbisPDF
    app/main.cpp
    core/PdfDocument.cpp core/PdfDocument.h
    core/PageModel.cpp core/PageModel.h
    core/PageImageProvider.cpp core/PageImageProvider.h
    core/AppLogger.cpp core/AppLogger.h
    core/EngineCapabilities.cpp core/EngineCapabilities.h
    core/IPdfBackend.h core/QtPdfBackend.cpp core/QtPdfBackend.h
    core/MemoryPolicy.cpp core/MemoryPolicy.h
    core/LanguageManager.cpp core/LanguageManager.h
)

set_source_files_properties(ui/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)

qt_add_qml_module(OrbisPDF
    URI PDFStudio
    VERSION 6.0
    QML_FILES ui/Main.qml
    RESOURCES assets/orbis-pdf-logo.svg
)

qt_add_resources(OrbisPDF orbis_i18n
    PREFIX "/"
    FILES
        i18n/en.json
        i18n/ar.json
)

target_link_libraries(OrbisPDF PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::QuickControls2 Qt6::Pdf
)

if(WIN32)
    set_target_properties(OrbisPDF PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(ANDROID)
    set_property(TARGET OrbisPDF APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    set_property(TARGET OrbisPDF PROPERTY QT_ANDROID_PACKAGE_NAME "org.orbispdf.app")
    set_property(TARGET OrbisPDF PROPERTY QT_ANDROID_APP_NAME "Orbis PDF")
    set_property(TARGET OrbisPDF PROPERTY QT_ANDROID_MIN_SDK_VERSION 28)
    set_property(TARGET OrbisPDF PROPERTY QT_ANDROID_TARGET_SDK_VERSION 36)
    set_property(TARGET OrbisPDF PROPERTY QT_ANDROID_COMPILE_SDK_VERSION 36)
    set_property(TARGET OrbisPDF PROPERTY QT_ANDROID_VERSION_CODE 60000)
    set_property(TARGET OrbisPDF PROPERTY QT_ANDROID_VERSION_NAME "6.0.0")
endif()

if(ANDROID)
    # qt_add_executable() produces a MODULE library on Android.
    # CMake requires an explicit LIBRARY destination for module targets, even
    # though APK/AAB packaging itself is performed by Qt's androiddeployqt flow.
    install(TARGETS OrbisPDF
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
else()
    install(TARGETS OrbisPDF
        BUNDLE DESTINATION .
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
