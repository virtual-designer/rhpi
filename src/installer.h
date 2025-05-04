#ifndef LPI_INSTALLER_H
#define LPI_INSTALLER_H

#include <adwaita.h>
#include <gtk/gtk.h>
#include "rpm.h"

#define PACKAGE_INSTALLER_TYPE_WINDOW (package_installer_window_get_type())
G_DECLARE_FINAL_TYPE(PackageInstallerWindow, package_installer_window, PACKAGE_INSTALLER, WINDOW, AdwApplicationWindow);

GtkWidget *package_installer_window_new(GtkApplication *app, const struct rpm_data *data);

#endif /* LPI_INSTALLER_H */