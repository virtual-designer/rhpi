desktop_in_in = org.onesoftnet.redhat.PackageInstaller.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = org.onesoftnet.redhat.PackageInstaller.desktop

iconsdir = $(datadir)/icons/hicolor/scalable/apps
icons_DATA = org.onesoftnet.redhat.PackageInstaller.svg

EXTRA_DIST = $(desktop_in_in) $(icons_DATA)

%.desktop: %.desktop.in
	@echo "  GEN      $@" && cat $< > $@ && echo -e "\nIcon=$(iconsdir)/org.onesoftnet.redhat.PackageInstaller.svg" >> $@

CLEANFILES = org.onesoftnet.redhat.PackageInstaller.desktop
