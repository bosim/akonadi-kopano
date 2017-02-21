cmake_minimum_required(VERSION 2.8.12)

project(kopanoresource)

find_package(ECM 5.19 REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings)
include(KDECompilerSettings)
include(KDECMakeSettings)
find_package(KF5KDELibs4Support)

set(KF5_VERSION "5.19")
set(QT_REQUIRED_VERSION "5.5")
set(AKONADI_VERSION "5.2.0")
set(KMIME_LIB_VERSION "5.2.0")

# QT5 package
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Network Widgets Xml DBus)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Xsltproc)

find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ConfigWidgets ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore)
find_package(KF5Contacts)
find_package(KF5Codecs)
find_package(KF5WindowSystem ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Wallet ${KF5_VERSION} CONFIG REQUIRED)

find_path(MAPI_INCLUDE_DIR mapi.h
          HINTS "/usr/include/mapi4linux")

find_path(KOPANO_INCLUDE_DIR zcdefs.h
          HINTS "/usr/include/kopano")

include_directories(
    ${MAPI_INCLUDE_DIR}
    ${KOPANO_INCLUDE_DIR}
)

set(kopanoresource_SRCS
  ItemAddedJob.cpp
  ItemsFlagsChangedJob.cpp
  ItemsMovedJob.cpp
  ItemsRemovedJob.cpp
  MailResource.cpp
  MapiSession.cpp
  RetrieveCollectionsJob.cpp
  RetrieveItemJob.cpp
  RetrieveItemsJob.cpp
  SendItemJob.cpp
  SettingsDialog.cpp
  Utils.cpp
)

ki18n_wrap_ui(kopanoresource_SRCS Settings.ui)
kconfig_add_kcfg_files(kopanoresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kopanoresource.kcfg org.kde.Akonadi.Kopano.Settings)
qt5_add_dbus_adaptor(kopanoresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Kopano.Settings.xml settings.h Settings
)

add_executable(akonadi_kopano_resource ${kopanoresource_SRCS})

target_link_libraries(akonadi_kopano_resource
  KF5::AkonadiCore
  KF5::AkonadiMime
  KF5::Mime
  KF5::CalendarCore   
  KF5::Contacts       
  KF5::AkonadiAgentBase
  KF5::AkonadiWidgets
  KF5::WindowSystem
  KF5::Wallet
  KF5::I18n
  mapi kcinetmapi 
  kcmapi kcutil
)

