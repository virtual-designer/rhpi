bin_PROGRAMS = rhpi
rhpi_SOURCES = main.c installer.c rpm.c rhpi.gresource.c installer.h resource.h rpm.h

rhpi_CFLAGS = $(GTK_CFLAGS) $(LIBADWAITA_CFLAGS) $(POLKIT_CFLAGS)
rhpi_LDADD = $(GTK_LIBS) $(LIBADWAITA_LIBS) $(POLKIT_LIBS)

rhpi.gresource.c: rhpi.gresource.xml installer.ui
	@echo "  GEN      rhpi.gresource.c" && $(GLIB_COMPILE_RESOURCES) --target=$@ --generate-source --sourcedir=$(srcdir) $(srcdir)/rhpi.gresource.xml

CLEANFILES = rhpi.gresource.c
EXTRA_DIST = rhpi.gresource.xml installer.ui