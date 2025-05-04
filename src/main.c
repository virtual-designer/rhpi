#include "installer.h"
#include "resource.h"

#include <adwaita.h>

static const char *argv_0 = NULL;

static gboolean help = FALSE;
static gboolean version = FALSE;
static gboolean is_cli = FALSE;

static GOptionEntry entries[] = {
    {"help",    'h', 0, G_OPTION_ARG_NONE, &help,    "Show this help message",   NULL},
    {"version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show version information",
     NULL                                                                            },
    {"cli",     'C', 0, G_OPTION_ARG_NONE, &is_cli,  "Run in CLI mode",          NULL},
    G_OPTION_ENTRY_NULL,
};

static void setup_css()
{
    GtkCssProvider *global_provider = gtk_css_provider_new();
    GtkCssProvider *color_scheme_provider = gtk_css_provider_new();
    GdkDisplay *display = gdk_display_get_default();
    AdwStyleManager *style_manager = adw_style_manager_get_for_display(display);
    gboolean dark_mode = adw_style_manager_get_dark(style_manager);

    gtk_css_provider_load_from_resource(global_provider, LPI_RES_CSS("global"));
    gtk_style_context_add_provider_for_display(
        display, GTK_STYLE_PROVIDER(global_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_print("Dark mode: %s\n", dark_mode ? "enabled" : "disabled");

    gtk_css_provider_load_from_resource(
        color_scheme_provider,
        dark_mode ? LPI_RES_CSS("dark") : LPI_RES_CSS("light"));

    gtk_style_context_add_provider_for_display(
        display, GTK_STYLE_PROVIDER(color_scheme_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static int on_command_line(
    GApplication *app, GApplicationCommandLine *cmdline, gpointer user_data)
{
    int argc = 0;
    gchar **argv = g_application_command_line_get_arguments(cmdline, &argc);
    GError *error = NULL;
    GOptionContext *context = g_option_context_new("[-h] [-v] [-C] <file.rpm>");

    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_set_summary(context, "Red Hat Package Installer.");
    g_option_context_set_description(
        context, "This GUI program installs RPM packages on Red Hat-based "
                 "Linux distributions.");
    g_option_context_set_help_enabled(context, TRUE);
    g_option_context_set_ignore_unknown_options(context, FALSE);

    g_option_context_parse(context, &argc, &argv, &error);

    if (error) {
        g_printerr("%s: %s\n", argv_0, error->message);
        g_error_free(error);
        return 1;
    }

    if (argc == 1) {
        g_printerr("%s: No arguments provided\n", argv_0);
        g_printerr("Try '%s --help' for more information.\n", argv_0);
        return 1;
    }

    if (argc > 2) {
        g_printerr("%s: Too many arguments provided\n", argv_0);
        g_printerr("Try '%s --help' for more information.\n", argv_0);
        return 1;
    }

    gchar *filename = argv[1];
    GFile *file = g_file_new_for_commandline_arg(filename);

    if (!file) {
        g_printerr("%s: %s: %s\n", argv_0, filename, (errno));
        return 1;
    }

    g_application_open(app, &file, 1, "");
    return 0;
}

static void
terminate_err(GObject *object, GAsyncResult *res, gpointer user_data)
{
    exit(1);
}

void debug_print_rpm_data(const struct rpm_data *data);

static void
on_open(GtkApplication *app, GFile **files, gint n_files, const char *hint)
{
    GFile *file = files[0];
    struct rpm_data *data = rpm_data_parse(g_file_get_path(file));
    const char *errmsg = NULL;

    debug_print_rpm_data(data);

    if (!data) {
        errmsg = "Failed to parse RPM file";
    } else if (data->errmsg) {
        errmsg = data->errmsg;
    }

    GtkWidget *installer_window = package_installer_window_new(app, data);
    setup_css();
    gtk_window_present(GTK_WINDOW(installer_window));

    if (!errmsg) {
        return;
    }

    g_printerr("%s: %s\n", argv_0, errmsg);

    if (is_cli) {
        exit(1);
    }

    AdwDialog *dialog = adw_alert_dialog_new("Package Installer Error", NULL);

    adw_alert_dialog_format_body(
        ADW_ALERT_DIALOG(dialog),
        "An error occurred while installing the package:\n\n%s",
        data->errmsg
    );

    adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog), "ok", "_OK", NULL);
    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "ok");
    adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "ok");
    g_signal_connect(dialog, "response", G_CALLBACK(terminate_err), NULL);
    adw_dialog_present(dialog, GTK_WIDGET(installer_window));
}

int main(int argc, char *argv[])
{
    argv_0 = argv[0];

    g_autoptr(AdwApplication) app = adw_application_new(
        "org.onesoftnet.redhat.PackageInstaller",
        G_APPLICATION_DEFAULT_FLAGS | G_APPLICATION_HANDLES_COMMAND_LINE |
            G_APPLICATION_HANDLES_OPEN);

    g_signal_connect(app, "command-line", G_CALLBACK(on_command_line), NULL);
    g_signal_connect(app, "open", G_CALLBACK(on_open), NULL);

    return g_application_run(G_APPLICATION(app), argc, argv);
}
