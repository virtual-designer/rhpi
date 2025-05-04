#define _GNU_SOURCE

#include "installer.h"

#include <adwaita.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <polkit/polkit.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

struct _PackageInstallerWindow {
    AdwApplicationWindow parent_instance;
    const struct rpm_data *data;

    GtkWidget *package_name_label;
    GtkWidget *package_file_name_label;
    GtkWidget *package_summary_label;
    GtkWidget *package_version_row;
    GtkWidget *package_arch_row;
    GtkWidget *package_size_row;
    GtkWidget *package_vendor_row;
    GtkWidget *package_build_date_row;
    GtkWidget *package_build_host_row;
    GtkWidget *package_packager_row;
    GtkWidget *package_license_row;
    GtkWidget *package_url_row;
    GtkWidget *package_bug_report_url_row;
    GtkWidget *package_description_row;

    GtkWidget *install_button;
    GtkWidget *package_info_list;
    GtkWidget *progress_container;
    GtkWidget *progress;

    GtkWidget *error_container;
    GtkWidget *error_label;
    GtkWidget *done_button;
};

G_DEFINE_TYPE(
    PackageInstallerWindow, package_installer_window,
    GTK_TYPE_APPLICATION_WINDOW)

static void
package_installer_window_class_init(PackageInstallerWindowClass *klass)
{
    gtk_widget_class_set_template_from_resource(
        GTK_WIDGET_CLASS(klass),
        "/org/onesoftnet/redhat/PackageInstaller/ui/installer.ui");

    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_name_label);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow,
        package_file_name_label);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_summary_label);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_version_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_arch_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_size_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_vendor_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow,
        package_build_date_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow,
        package_build_host_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_packager_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_license_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_url_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow,
        package_bug_report_url_row);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow,
        package_description_row);

    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, install_button);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, package_info_list);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, progress_container);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, progress);

    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, error_container);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, error_label);
    gtk_widget_class_bind_template_child(
        GTK_WIDGET_CLASS(klass), PackageInstallerWindow, done_button);
}

static void package_installer_window_init(PackageInstallerWindow *self)
{
    gtk_widget_init_template(GTK_WIDGET(self));
}

static void show_error(PackageInstallerWindow *self, const char *error)
{
    gtk_widget_set_visible(self->progress_container, FALSE);
    gtk_widget_set_visible(self->package_info_list, FALSE);
    gtk_widget_set_visible(self->package_summary_label, FALSE);

    gtk_label_set_text(GTK_LABEL(self->error_label), error);
    gtk_widget_set_visible(self->error_container, TRUE);
}

static gboolean pulse_progress(gpointer user_data) {
    GtkProgressBar *bar = GTK_PROGRESS_BAR(user_data);
    gtk_progress_bar_pulse(bar);
    return G_SOURCE_CONTINUE;
}

static void *perform_installation(void *data)
{
    PackageInstallerWindow *self = (PackageInstallerWindow *)data;

    if (!self || !self->data) {
        show_error(self, "Invalid data");
        return NULL;
    }

    guint timer_id = g_timeout_add(300, &pulse_progress, self->progress);
    const gchar *HOME = g_get_home_dir();
    const gchar *cache_dir = g_strdup_printf("%s/.cache", HOME == NULL ? "/tmp" : HOME);

    if (!g_file_test(cache_dir, G_FILE_TEST_EXISTS)) {
        g_mkdir_with_parents(cache_dir, 0700);
    }

    g_print("Cache directory: %s\n", cache_dir);

    const gchar *log_file = g_strdup_printf("%s/install.log", cache_dir);
    FILE *log_fp = fopen(log_file, "a");

    if (!log_fp) {
        show_error(self, "Failed to open log file");
        return NULL;
    }

    char *command =
        g_strdup_printf("export SUDO_USER=1; dnf install -y %s; exit $?;", self->data->filename);

    int stdout_fd[2];
    int stderr_fd[2];

    if (pipe(stdout_fd) == -1 || pipe(stderr_fd) == -1) {
        show_error(self, "Failed to create pipes");
        return NULL;
    }

    pid_t pid = fork();

    if (pid == -1) {
        show_error(self, "Failed to fork process");
        return NULL;
    }

    if (pid == 0) {
        // Child process
        dup2(stdout_fd[1], STDOUT_FILENO);
        dup2(stderr_fd[1], STDERR_FILENO);
        close(stdout_fd[0]);
        close(stderr_fd[0]);
        close(stdout_fd[1]);
        close(stderr_fd[1]);

        execlp("pkexec", "pkexec", "sh", "-c", command, NULL);
        _exit(127);
    } else {
        close(stdout_fd[1]);
        close(stderr_fd[1]);

        FILE *stdout_fp = fdopen(stdout_fd[0], "r");
        FILE *stderr_fp = fdopen(stderr_fd[0], "r");

        char buffer[64];
        ssize_t bytes_read = 0;
        int status;

        while (!feof(stdout_fp) &&
               (bytes_read = fread(buffer, 1, sizeof(buffer) - 1, stdout_fp)) >
                   0) {
            buffer[bytes_read] = '\0';
            g_print("dnf: %s", buffer);
            fprintf(log_fp, "%s\n", buffer);
        }

        if (bytes_read == -1 && !feof(stdout_fp)) {
            show_error(self, "Failed to read from stdout");
        }

        bytes_read = 0;
        bzero(buffer, sizeof(buffer));

        while (!feof(stderr_fp) &&
               (bytes_read = fread(buffer, 1, sizeof(buffer) - 1, stderr_fp)) >
                   0) {
            buffer[bytes_read] = '\0';
            g_print("dnf error: %s", buffer);
            fprintf(log_fp, "%s\n", buffer);
        }

        if (bytes_read == -1 && !feof(stderr_fp)) {
            show_error(self, "Failed to read from stderr");
        }

        waitpid(pid, &status, 0);

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            show_error(self, "Installation failed");
        }
        
        fclose(log_fp);
        fclose(stdout_fp);
        fclose(stderr_fp);
        
        g_source_remove(timer_id);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->progress), 1.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(self->progress), "Installation complete");

        int code = 0;
        g_signal_connect(self->done_button, "clicked", G_CALLBACK(exit), &code);
        
        gtk_widget_set_visible(self->done_button, TRUE);
    }
 
    return NULL;
}

static void on_install_button_clicked(GtkWidget *widget, gpointer user_data)
{
    PackageInstallerWindow *self = PACKAGE_INSTALLER_WINDOW(user_data);
    const struct rpm_data *data = self->data;

    if (!data) {
        return;
    }

    gtk_widget_set_visible(self->install_button, FALSE);
    gtk_widget_set_visible(self->package_info_list, FALSE);
    gtk_widget_set_visible(self->package_summary_label, FALSE);

    gtk_widget_set_visible(self->progress_container, TRUE);

    GThread *thread = g_thread_new(
        "install_thread", &perform_installation, self);

    if (!thread) {
        show_error(self, "Failed to create thread");
        return;
    }
}

GtkWidget *
package_installer_window_new(GtkApplication *app, const struct rpm_data *data)
{
    PackageInstallerWindow *self =
        g_object_new(PACKAGE_INSTALLER_TYPE_WINDOW, "application", app, NULL);

    self->data = data;

    if (!self->data || self->data->errmsg) {
        gtk_window_set_default_size(
            GTK_WINDOW(&self->parent_instance), 400, 485);
        gtk_window_set_child(
            GTK_WINDOW(&self->parent_instance), gtk_label_new(""));
        return GTK_WIDGET(self);
    }

    gtk_label_set_text(GTK_LABEL(self->package_name_label), self->data->name);
    gtk_label_set_text(
        GTK_LABEL(self->package_file_name_label),
        g_path_get_basename(self->data->filename));
    gtk_label_set_text(
        GTK_LABEL(self->package_summary_label), self->data->summary);

    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_version_row), self->data->version);
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_arch_row), self->data->arch);
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_size_row),
        g_format_size(self->data->size));
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_vendor_row), self->data->vendor);
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_build_date_row),
        g_strdup_printf("%s", ctime(&self->data->build_date)));
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_build_host_row), self->data->build_host);
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_packager_row), self->data->packager);
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_license_row), self->data->license);
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_url_row), self->data->url);
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_bug_report_url_row), self->data->bug_url);
    adw_action_row_set_subtitle(
        ADW_ACTION_ROW(self->package_description_row), self->data->description);

    g_signal_connect(
        self->install_button, "clicked", G_CALLBACK(on_install_button_clicked),
        self);

    return GTK_WIDGET(self);
}