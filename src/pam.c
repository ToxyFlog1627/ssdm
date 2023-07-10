#define _GNU_SOURCE
#include "pam.h"
#include <assert.h>
#include <security/pam_appl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>

#define PAM_CLOSE()                  \
    {                                \
        pam_end(pam_handle, status); \
        pam_handle = NULL;           \
    }

pam_handle_t *pam_handle = NULL;

int conv(int num_msg, const struct pam_message **msgs, struct pam_response **res, void *appdata) {
    *res = calloc(num_msg, sizeof(struct pam_response));
    if (*res == NULL) return PAM_BUF_ERR;

    for (int i = 0; i < num_msg; i++) {
        switch (msgs[i]->msg_style) {
            case PAM_PROMPT_ECHO_ON:
                (*res)[i].resp = strdup(((char **) appdata)[0]);
                break;
            case PAM_PROMPT_ECHO_OFF:
                (*res)[i].resp = strdup(((char **) appdata)[1]);
                break;
            case PAM_ERROR_MSG:
                syslog(LOG_ERR, "PAM conv error message: \"%s\"", msgs[i]->msg);
                free(*res);
                return PAM_CONV_ERR;
            case PAM_TEXT_INFO:
                syslog(LOG_INFO, "PAM conv info message: \"%s\"", msgs[i]->msg);
                break;
            default:
                syslog(LOG_EMERG, "Unkown message type in PAM conv");
                exit(EXIT_FAILURE);
                break;
        }
    }

    return PAM_SUCCESS;
}

int pam_login(const char *username, const char *password) {
    assert(username != NULL && password != NULL && username[0] != '\0' && password[0] != '\0' && pam_handle == NULL);

    const char *data[2] = {username, password};
    struct pam_conv pam_conv = {conv, data};

    int status = pam_start("ssdm", NULL, &pam_conv, &pam_handle);
    if (status != PAM_SUCCESS) return AUTH_ERROR;
    status = pam_authenticate(pam_handle, 0);
    if (status == PAM_AUTH_ERR || status == PAM_PERM_DENIED) {
        PAM_CLOSE();
        return AUTH_WRONG_CREDENTIALS;
    }
    if (status != PAM_SUCCESS) goto on_error;
    status = pam_acct_mgmt(pam_handle, 0);
    if (status != PAM_SUCCESS) goto on_error;
    status = pam_setcred(pam_handle, PAM_ESTABLISH_CRED);
    if (status != PAM_SUCCESS) goto on_error;
    status = pam_open_session(pam_handle, 0);
    if (status != PAM_SUCCESS) goto on_error;

    return AUTH_SUCCESS;
on_error:
    PAM_CLOSE();
    return AUTH_ERROR;
}

int pam_logout(void) {
    if (pam_handle == NULL) return AUTH_SUCCESS;

    int status = pam_close_session(pam_handle, 0);
    if (status != PAM_SUCCESS) goto on_error;
    status = pam_setcred(pam_handle, PAM_DELETE_CRED);
    if (status != PAM_SUCCESS) goto on_error;
    status = pam_end(pam_handle, status);
    pam_handle = NULL;
    if (status != PAM_SUCCESS) return AUTH_ERROR;

    return AUTH_SUCCESS;
on_error:
    PAM_CLOSE();
    return AUTH_ERROR;
}

void pam_init_env(void) {
    assert(pam_handle != NULL);

    char **env = pam_getenvlist(pam_handle);
    if (env == NULL) {
        syslog(LOG_CRIT, "Unable to set PAM environment variables");
        return;
    }

    for (int i = 0; env[i] != NULL; i++) putenv(env[i]);
    free(env);
}