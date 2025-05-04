#ifndef LPI_RPM_H
#define LPI_RPM_H

#include <time.h>
#include <stddef.h>

struct rpm_data {
    char *filename;

    char *name;
    char *version;
    char *arch;
    char *summary;
    char *description;
    char *vendor;
    char *packager;
    char *url;
    char *license;
    char *bug_url;
    time_t build_date;
    char *build_host;
    
    size_t size;

    char *errmsg;
};

struct rpm_data *rpm_data_parse(const char *filename);
void rpm_data_destroy(struct rpm_data *data);

#endif /* LPI_RPM_H */