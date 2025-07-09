# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

# Target: static library
qt_add_qml_module(kwallets_static
    STATIC
    URI org.kde.kwallets
    VERSION 1.0
    QML_FILES
        contents/ui/Main.qml
        contents/ui/About.qml
        contents/ui/EntryPage.qml
        contents/ui/FormItem.qml
        contents/ui/MapField.qml
        contents/ui/CollectionContentsPage.qml
        contents/ui/CollectionListPage.qml
)

target_sources(kwallets_static PUBLIC
    app.cpp
    collectionmodel.cpp
    secretitemproxy.cpp
    secretserviceclient.cpp
    statetracker.cpp
    collectionsmodel.cpp
    resources.qrc
)

ecm_qt_declare_logging_category(kwallets_static
    HEADER kwallets_debug.h
    IDENTIFIER KWALLETS_LOG
    CATEGORY_NAME org.kde.kwallets
    DESCRIPTION "kwallets"
    EXPORT KWALLETS
)

target_link_libraries(kwallets_static PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    KF6::I18n
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    Qt::DBus
    ${LIBSECRET_LIBRARIES}
)
target_include_directories(kwallets_static PUBLIC ${CMAKE_BINARY_DIR})

if (ANDROID)
    kirigami_package_breeze_icons(ICONS
        list-add
        help-about
        application-exit
        applications-graphics
    )
else()
    target_link_libraries(kwallets_static PUBLIC Qt::Widgets)
endif()

kconfig_add_kcfg_files(kwallets_static GENERATE_MOC kwalletsconfig.kcfgc)

# Target: main executable
add_executable(kwallets main.cpp)
target_link_libraries(kwallets PUBLIC kwallets_static kwallets_staticplugin)
install(TARGETS kwallets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
