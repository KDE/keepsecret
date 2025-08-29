# SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(keepsecret VERSION 0.1)

set(QT6_MIN_VERSION 6.8.0)
set(KF6_MIN_VERSION 6.8.0)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMFindQmlModule)
include(ECMQtDeclareLoggingCategory)
include(KDEGitCommitHooks)
include(KDEClangFormat)
include(ECMDeprecationSettings)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KEEPSECRET
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/version-keepsecret.h"
)

find_package(Qt6 ${QT6_MIN_VERSION} REQUIRED COMPONENTS Core Gui Qml QuickControls2 Svg)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Kirigami CoreAddons Config I18n ItemModels)

if (NOT ANDROID AND NOT WIN32 AND NOT APPLE AND NOT HAIKU)
    find_package(KF6DBusAddons ${KF6_MIN_VERSION} REQUIRED)
endif()

if (NOT ANDROID)
    find_package(Qt6Widgets ${QT6_MIN_VERSION} REQUIRED)
endif()

qt_policy(SET QTP0001 NEW)
# They clash with glib signals
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DQT_NO_SIGNALS_SLOTS_KEYWORDS)
add_definitions(-DQT_NO_CONTEXTLESS_CONNECT)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSECRET IMPORTED_TARGET REQUIRED libsecret-1)

ecm_find_qmlmodule(org.kde.kirigamiaddons.formcard 1.0)

ecm_set_disabled_deprecation_versions(QT 6.10.0
    KF 6.17.0
)

add_subdirectory(src)

install(FILES org.kde.keepsecret.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.keepsecret.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
ki18n_install(po)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

ecm_qt_install_logging_categories(
    EXPORT keepsecret
    FILE keepsecret.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
