#ifndef TEDIT_APP_INFO_H
#define TEDIT_APP_INFO_H

#define TEDIT_APP_NAME "TEdit"
#define TEDIT_APP_VERSION "0.1.0"
#define TEDIT_APP_VERSION_TITLE TEDIT_APP_NAME " v" TEDIT_APP_VERSION
#define TEDIT_APP_COPYRIGHT "\xC2\xA9 RePass Cloud Pty Ltd 2026"

static inline const char *tedit_app_name(void) {
    return TEDIT_APP_NAME;
}

static inline const char *tedit_app_version(void) {
    return TEDIT_APP_VERSION;
}

static inline const char *tedit_app_version_title(void) {
    return TEDIT_APP_VERSION_TITLE;
}

static inline const char *tedit_app_copyright(void) {
    return TEDIT_APP_COPYRIGHT;
}

#endif
