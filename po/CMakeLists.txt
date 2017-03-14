find_package(Intltool REQUIRED)
set(GETTEXT_PACKAGE ${PROJECT_NAME})

intltool_update_potfile(
  GETTEXT_PACKAGE ${GETTEXT_PACKAGE}
  COPYRIGHT_HOLDER "Canonical Ltd."
  UBUNTU_SDK_DEFAULTS
  FILTER "tests/*"
)

intltool_install_translations(
  ALL
  GETTEXT_PACKAGE ${GETTEXT_PACKAGE}
)
