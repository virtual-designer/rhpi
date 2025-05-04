#ifndef LPI_RESOURCE_H
#define LPI_RESOURCE_H

#define LPI_RES_PREFIX "/org/onesoftnet/redhat/PackageInstaller"
#define LPI_RES_UI_PREFIX LPI_RES_PREFIX "/ui"
#define LPI_RES_CSS_PREFIX LPI_RES_PREFIX "/styles"

#define LPI_RES(name) LPI_RES_PREFIX "/" name
#define LPI_RES_UI(name) LPI_RES_UI_PREFIX "/" name ".ui"
#define LPI_RES_CSS(name) LPI_RES_CSS_PREFIX "/" name ".css"

#endif /* LPI_RESOURCE_H */