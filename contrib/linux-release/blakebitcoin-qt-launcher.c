#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef BLAKEBITCOIN_QT_LAUNCH_TARGET
#define BLAKEBITCOIN_QT_LAUNCH_TARGET ".runtime/blakebitcoin-qt-bin"
#endif

#ifndef BLAKEBITCOIN_QT_USE_RUNTIME_ENV
#define BLAKEBITCOIN_QT_USE_RUNTIME_ENV 1
#endif

#ifndef BLAKEBITCOIN_QT_LIBRARY_PATH
#define BLAKEBITCOIN_QT_LIBRARY_PATH ".runtime/lib"
#endif

#ifndef BLAKEBITCOIN_QT_PLUGIN_PATH
#define BLAKEBITCOIN_QT_PLUGIN_PATH ".runtime/plugins"
#endif

#ifndef BLAKEBITCOIN_QT_PLATFORM_PLUGIN_PATH
#define BLAKEBITCOIN_QT_PLATFORM_PLUGIN_PATH ".runtime/plugins/platforms"
#endif

#if BLAKEBITCOIN_QT_USE_RUNTIME_ENV
static int prepend_env_path(const char *name, const char *prefix) {
    const char *current = getenv(name);
    size_t prefix_len = strlen(prefix);
    size_t current_len = current ? strlen(current) : 0;
    size_t total_len = prefix_len + (current_len ? 1 + current_len : 0) + 1;
    char *value = malloc(total_len);

    if (value == NULL) {
        return -1;
    }

    if (current_len) {
        snprintf(value, total_len, "%s:%s", prefix, current);
    } else {
        snprintf(value, total_len, "%s", prefix);
    }

    if (setenv(name, value, 1) != 0) {
        free(value);
        return -1;
    }

    free(value);
    return 0;
}
#endif

int main(int argc, char **argv) {
    char self_path[PATH_MAX];
    char app_dir[PATH_MAX];
    char target_path[PATH_MAX];
#if BLAKEBITCOIN_QT_USE_RUNTIME_ENV
    char lib_path[PATH_MAX];
    char plugin_path[PATH_MAX];
    char platform_plugin_path[PATH_MAX];
#endif
    char **exec_argv = NULL;
    ssize_t len;
    char *slash;
    int i;

    len = readlink("/proc/self/exe", self_path, sizeof(self_path) - 1);
    if (len < 0) {
        perror("readlink");
        return 1;
    }
    self_path[len] = '\0';

    if (snprintf(app_dir, sizeof(app_dir), "%s", self_path) >= (int)sizeof(app_dir)) {
        fprintf(stderr, "Launcher path is too long\n");
        return 1;
    }

    slash = strrchr(app_dir, '/');
    if (slash == NULL) {
        fprintf(stderr, "Could not determine launcher directory\n");
        return 1;
    }
    *slash = '\0';

    if (snprintf(target_path, sizeof(target_path), "%s/%s", app_dir, BLAKEBITCOIN_QT_LAUNCH_TARGET) >= (int)sizeof(target_path)) {
        fprintf(stderr, "Runtime path is too long\n");
        return 1;
    }

#if BLAKEBITCOIN_QT_USE_RUNTIME_ENV
    if (snprintf(lib_path, sizeof(lib_path), "%s/%s", app_dir, BLAKEBITCOIN_QT_LIBRARY_PATH) >= (int)sizeof(lib_path) ||
        snprintf(plugin_path, sizeof(plugin_path), "%s/%s", app_dir, BLAKEBITCOIN_QT_PLUGIN_PATH) >= (int)sizeof(plugin_path) ||
        snprintf(platform_plugin_path, sizeof(platform_plugin_path), "%s/%s", app_dir, BLAKEBITCOIN_QT_PLATFORM_PLUGIN_PATH) >= (int)sizeof(platform_plugin_path)) {
        fprintf(stderr, "Runtime path is too long\n");
        return 1;
    }

    if (prepend_env_path("LD_LIBRARY_PATH", lib_path) != 0 ||
        prepend_env_path("QT_PLUGIN_PATH", plugin_path) != 0) {
        perror("setenv");
        return 1;
    }

    if (setenv("QT_QPA_PLATFORM_PLUGIN_PATH", platform_plugin_path, 1) != 0) {
        perror("setenv");
        return 1;
    }
#endif

    exec_argv = calloc((size_t)argc + 1, sizeof(char *));
    if (exec_argv == NULL) {
        perror("calloc");
        return 1;
    }

    exec_argv[0] = target_path;
    for (i = 1; i < argc; ++i) {
        exec_argv[i] = argv[i];
    }
    exec_argv[argc] = NULL;

    execv(target_path, exec_argv);

    fprintf(stderr, "Failed to launch %s: %s\n", target_path, strerror(errno));
    free(exec_argv);
    return 1;
}
