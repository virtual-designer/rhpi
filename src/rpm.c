/*
 * rpm.c -- Handle Red Hat Package Manager (RPM) files
 *
 * This file is part of the LPI Installer.
 * Copyright (C) 2025 Ar Rakin <rakinar2@onesoftnet.eu.org>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include "rpm.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static struct rpm_data *rpm_data_create(void)
{
    struct rpm_data *data = calloc(1, sizeof(struct rpm_data));

    if (!data) {
        return NULL;
    }

    return data;
}

void rpm_data_destroy(struct rpm_data *data)
{
    if (!data) {
        return;
    }

    free(data->name);
    free(data->version);
    free(data->arch);
    free(data->summary);
    free(data->description);
    free(data->vendor);
    free(data->packager);
    free(data->url);
    free(data->license);
    free(data->bug_url);
    free(data->build_host);

    free(data->errmsg);
    free(data->filename);
    free(data);
}

bool rand_sep(char *sep)
{
    FILE *fp = fopen("/dev/urandom", "r");

    if (!fp) {
        return false;
    }

    char buf[32];
    size_t len = fread(buf, 1, 32, fp);

    if (len != 32) {
        fclose(fp);
        return false;
    }

    fclose(fp);

    for (size_t i = 0; i < 32; i++) {
        char nibble1 = (buf[i] & 0xF0) >> 4;
        char nibble2 = buf[i] & 0x0F;

        sep[i * 2] = (nibble1 < 10) ? '0' + nibble1 : 'a' + (nibble1 - 10);
        sep[i * 2 + 1] = (nibble2 < 10) ? '0' + nibble2 : 'a' + (nibble2 - 10);
    }

    sep[64] = 0;
    return true;
}

struct str_split {
    char **array;
    size_t count;
};

static struct str_split split_string(const char *str, const char *sep)
{
    char **result = NULL;
    size_t len = strlen(str);
    size_t i = 0, r_index = 0, last_sep_index = 0;
    size_t sep_len = strlen(sep);

    while (i < len) {
        if (strncmp(str + i, sep, sep_len) == 0 || (len > 0 && i == len - 1)) {
            result = realloc(result, sizeof(char *) * (r_index + 1));
            result[r_index++] =
                strndup(str + last_sep_index, i - last_sep_index);
            last_sep_index = i + sep_len;
            i += sep_len;
            continue;
        }

        i++;
    }

    return (struct str_split) {.array = result, .count = r_index};
}

struct rpm_data *rpm_data_parse(const char *filename)
{
    struct rpm_data *data = rpm_data_create();

    if (!data) {
        return NULL;
    }

    int stdout_pipefd[2];
    int stderr_pipefd[2];

    if (pipe(stdout_pipefd) == -1) {
        data->errmsg = strdup("Failed to create pipe");
        return data;
    }

    if (pipe(stderr_pipefd) == -1) {
        data->errmsg = strdup("Failed to create pipe");
        return data;
    }

    char sep[65] = {0};

    if (!rand_sep(sep)) {
        data->errmsg = strdup("Failed to allocate memory");
        close(stdout_pipefd[0]);
        close(stdout_pipefd[1]);
        close(stderr_pipefd[0]);
        close(stderr_pipefd[1]);
        return data;
    }

    pid_t pid = fork();

    if (pid == 0) {
        close(stdout_pipefd[0]);
        dup2(stdout_pipefd[1], STDOUT_FILENO);
        close(stdout_pipefd[1]);

        close(stderr_pipefd[0]);
        dup2(stderr_pipefd[1], STDERR_FILENO);
        close(stderr_pipefd[1]);

        char *args[] = {"rpm", "-qp", "--queryformat", NULL, (char *) filename,
                        NULL};

        asprintf(
            &args[3],
            "%%{NAME}%s%%{VERSION}-%%{RELEASE}%s%%{ARCH}%s%%{VENDOR}%s%%{"
            "PACKAGER}"
            "%s%%{URL}%s%%{LICENSE}%s%%{BUGURL}%s%%{BUILDHOST}"
            "%s%%{SUMMARY}%s%%{BUILDTIME}%s%%{SIZE}%s%%{DESCRIPTION}",
            sep, sep, sep, sep, sep, sep, sep, sep, sep, sep, sep, sep);

        execvp(args[0], args);
        fprintf(stderr, "execve failed: %s\n", strerror(errno));
        exit(1);
    } else if (pid < 0) {
        data->errmsg = strdup("Failed to fork process");
        return data;
    }

    int status;

    close(stdout_pipefd[1]);
    close(stderr_pipefd[1]);
    waitpid(pid, &status, 0);

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        char *err_msg = NULL;
        FILE *err_fp = fdopen(stderr_pipefd[0], "r");

        if (err_fp) {
            char *line = NULL;
            size_t len = 0;
            ssize_t read = getline(&line, &len, err_fp);

            if (read != -1) {
                line[read - 1] = '\0';
                err_msg = strdup(line);
            }

            fclose(err_fp);
        }

        asprintf(
            &data->errmsg, "RPM command failed with status %d: %s",
            WEXITSTATUS(status), err_msg);
        close(stdout_pipefd[0]);
        close(stderr_pipefd[0]);
        free(err_msg);
        return data;
    }

    FILE *rpm_fp = fdopen(stdout_pipefd[0], "r");
    char *output = NULL;
    size_t output_len = 0;

    if (!rpm_fp) {
        data->errmsg = strdup("Failed to open pipe");
        return data;
    }

    while (!feof(rpm_fp)) {
        char buffer[1024];
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), rpm_fp);

        if (bytes_read == 0) {
            if (ferror(rpm_fp)) {
                data->errmsg = strdup("Failed to read from pipe");
                free(output);
                fclose(rpm_fp);
                return data;
            }

            break;
        }

        char *new_output = realloc(output, output_len + bytes_read + 1);

        if (!new_output) {
            data->errmsg = strdup("Failed to allocate memory for output");
            free(output);
            fclose(rpm_fp);
            return data;
        }

        output = new_output;
        memcpy(output + output_len, buffer, bytes_read);
        output_len += bytes_read;
        output[output_len] = '\0';
    }

    output = realloc(output, output_len + 1);
    output[output_len] = 0;

    fclose(rpm_fp);

    char **fields_list[] = {
        &data->name,       &data->version, &data->arch,    &data->vendor,
        &data->packager,   &data->url,     &data->license, &data->bug_url,
        &data->build_host, &data->summary,
    };
    size_t fields_count = sizeof(fields_list) / sizeof(fields_list[0]);
    struct str_split split = split_string(output, sep);

    if (split.count < fields_count) {
        data->errmsg = strdup("Not enough fields in RPM output");
        free(output);
        return data;
    }

    for (size_t i = 0; i < fields_count; i++) {
        *fields_list[i] = split.array[i];
    }

    data->build_date = (time_t) strtol(split.array[fields_count], NULL, 10);
    data->size = (size_t) strtoul(split.array[fields_count + 1], NULL, 10);
    data->description = strdup(split.array[fields_count + 2]);
    data->errmsg = NULL;

    for (size_t i = fields_count; i < split.count; i++) {
        free(split.array[i]);
    }

    free(output);
    data->filename = strdup(filename);
    return data;
}

void debug_print_rpm_data(const struct rpm_data *data)
{
    if (!data) {
        return;
    }

    printf("Filename: %s\n", data->filename);
    printf("Name: %s\n", data->name);
    printf("Version: %s\n", data->version);
    printf("Arch: %s\n", data->arch);
    printf("Summary: %s\n", data->summary);
    printf("Description: %s\n", data->description);
    printf("Vendor: %s\n", data->vendor);
    printf("Packager: %s\n", data->packager);
    printf("URL: %s\n", data->url);
    printf("License: %s\n", data->license);
    printf("Bug URL: %s\n", data->bug_url);
    printf("Build Date: %ld\n", (long) data->build_date);
    printf("Build Host: %s\n", data->build_host);
    printf("Size: %zu bytes\n", data->size);

    if (data->errmsg) {
        fprintf(stderr, "Error message: %s\n", data->errmsg);
    }
}