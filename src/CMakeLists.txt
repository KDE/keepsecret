# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

add_executable(keepsecret)
qt_add_qml_module(keepsecret URI org.kde.keepsecret
    QML_FILES
        qml/Main.qml
        qml/About.qml
        qml/EntryPage.qml
        qml/FormItem.qml
        qml/MapField.qml
        qml/CollectionContentsPage.qml
        qml/CollectionListPage.qml
)

target_sources(keepsecret PRIVATE
    main.cpp
    app.cpp
    collectionmodel.cpp
    secretitemproxy.cpp
    secretserviceclient.cpp
    statetracker.cpp
    collectionsmodel.cpp
    resources.qrc
)

ecm_qt_declare_logging_category(keepsecret
    HEADER keepsecret_debug.h
    IDENTIFIER KEEPSECRET_LOG
    CATEGORY_NAME org.kde.keepsecret
    DESCRIPTION "keepsecret"
    EXPORT keepsecret
)

target_include_directories(keepsecret PRIVATE
    ${CMAKE_BINARY_DIR}
    ${LIBSECRET_INCLUDE_DIRS}
)
target_link_libraries(keepsecret PRIVATE
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

if (ANDROID)
    kirigami_package_breeze_icons(ICONS
        list-add
        help-about
        application-exit
        applications-graphics
    )
else()
    target_link_libraries(keepsecret PRIVATE Qt::Widgets)
endif()

kconfig_target_kcfg_file(keepsecret
    FILE keepsecretconfig.kcfg
    CLASS_NAME keepsecretConfig
    GENERATE_MOC
    MUTATORS
    DEFAULT_VALUE_GETTERS
    GENERATE_PROPERTIES
    PARENT_IN_CONSTRUCTOR
    SINGLETON
)

install(TARGETS keepsecret ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
