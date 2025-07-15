# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

# Target: static library
qt_add_qml_module(keepsecret_static
    STATIC
    URI org.kde.keepsecret
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/About.qml
        qml/EntryPage.qml
        qml/FormItem.qml
        qml/MapField.qml
        qml/CollectionContentsPage.qml
        qml/CollectionListPage.qml
)

target_sources(keepsecret_static PUBLIC
    app.cpp
    collectionmodel.cpp
    secretitemproxy.cpp
    secretserviceclient.cpp
    statetracker.cpp
    collectionsmodel.cpp
    resources.qrc
)

ecm_qt_declare_logging_category(keepsecret_static
    HEADER keepsecret_debug.h
    IDENTIFIER KEEPSECRET_LOG
    CATEGORY_NAME org.kde.keepsecret
    DESCRIPTION "keepsecret"
    EXPORT keepsecret
)

target_link_libraries(keepsecret_static PUBLIC
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
target_include_directories(keepsecret_static PUBLIC ${CMAKE_BINARY_DIR})

if (ANDROID)
    kirigami_package_breeze_icons(ICONS
        list-add
        help-about
        application-exit
        applications-graphics
    )
else()
    target_link_libraries(keepsecret_static PUBLIC Qt::Widgets)
endif()

kconfig_add_kcfg_files(keepsecret_static GENERATE_MOC keepsecretconfig.kcfgc)

# Target: main executable
add_executable(keepsecret main.cpp)
target_link_libraries(keepsecret PUBLIC keepsecret_static keepsecret_staticplugin)
install(TARGETS keepsecret ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
