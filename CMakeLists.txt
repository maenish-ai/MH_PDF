cmake_minimum_required(VERSION 3.21)
project(MaenPDF VERSION 7.4.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 Pdf Concurrent)
qt_standard_project_setup(REQUIRES 6.11)

qt_add_executable(MaenPDF
    app/main.cpp
    core/PdfDocument.cpp core/PdfDocument.h
    core/PageModel.cpp core/PageModel.h
    core/PageImageProvider.cpp core/PageImageProvider.h
    core/AppLogger.cpp core/AppLogger.h
    core/EngineCapabilities.cpp core/EngineCapabilities.h
    core/IPdfBackend.h core/QtPdfBackend.cpp core/QtPdfBackend.h
    core/MemoryPolicy.cpp core/MemoryPolicy.h
    core/LanguageManager.cpp core/LanguageManager.h
    core/AppSettings.cpp core/AppSettings.h
    core/DocumentManager.cpp core/DocumentManager.h
    core/SessionImageProvider.cpp core/SessionImageProvider.h
    core/PdfToolsService.cpp core/PdfToolsService.h
    core/PrintService.cpp core/PrintService.h
)

set_source_files_properties(ui/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)

qt_add_qml_module(MaenPDF
    URI PDFStudio
    VERSION 7.0
    QML_FILES ui/Main.qml
    RESOURCES assets/maenpdf-logo.svg
)

qt_add_resources(MaenPDF maenpdf_i18n
    PREFIX "/"
    FILES
        i18n/en.json
        i18n/ar.json
)

target_link_libraries(MaenPDF PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::QuickControls2 Qt6::Pdf Qt6::Concurrent
)

# Release hardening. MaenPDF is open source, so security must come from safe
# design and platform mitigations rather than hidden code or embedded secrets.
if(MSVC)
    target_compile_options(MaenPDF PRIVATE $<$<CONFIG:Release>:/guard:cf> $<$<CONFIG:Release>:/sdl>)
    target_link_options(MaenPDF PRIVATE $<$<CONFIG:Release>:/guard:cf> $<$<CONFIG:Release>:/DYNAMICBASE> $<$<CONFIG:Release>:/NXCOMPAT>)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(MaenPDF PRIVATE $<$<CONFIG:Release>:-fstack-protector-strong> $<$<CONFIG:Release>:-fno-strict-overflow>)
    if(UNIX AND NOT APPLE)
        target_link_options(MaenPDF PRIVATE $<$<CONFIG:Release>:-Wl,-z,relro> $<$<CONFIG:Release>:-Wl,-z,now>)
    endif()
endif()

if(NOT ANDROID)
    find_package(Qt6 6.11 REQUIRED COMPONENTS Widgets PrintSupport)
    target_link_libraries(MaenPDF PRIVATE Qt6::Widgets Qt6::PrintSupport)
endif()

if(WIN32)
    set_target_properties(MaenPDF PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "MaenPDF"
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/MaenPDF.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/MaenPDF.rc"
        @ONLY
    )
    target_sources(MaenPDF PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/MaenPDF.rc")
endif()

if(ANDROID)
    set_property(TARGET MaenPDF APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    set_property(TARGET MaenPDF PROPERTY QT_ANDROID_PACKAGE_NAME "org.orbispdf.app")
    set_property(TARGET MaenPDF PROPERTY QT_ANDROID_APP_NAME "MaenPDF")
    set_property(TARGET MaenPDF PROPERTY QT_ANDROID_MIN_SDK_VERSION 28)
    set_property(TARGET MaenPDF PROPERTY QT_ANDROID_TARGET_SDK_VERSION 36)
    set_property(TARGET MaenPDF PROPERTY QT_ANDROID_COMPILE_SDK_VERSION 36)
    set_property(TARGET MaenPDF PROPERTY QT_ANDROID_VERSION_CODE 70400)
    set_property(TARGET MaenPDF PROPERTY QT_ANDROID_VERSION_NAME "7.4.0")
endif()

if(ANDROID)
    # qt_add_executable() produces a MODULE library on Android.
    # CMake requires an explicit LIBRARY destination for module targets, even
    # though APK/AAB packaging itself is performed by Qt's androiddeployqt flow.
    install(TARGETS MaenPDF
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
else()
    install(TARGETS MaenPDF
        BUNDLE DESTINATION .
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
endif()
