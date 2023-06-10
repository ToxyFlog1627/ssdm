#define _DEFAULT_SOURCE
#include "pam.h"
#include <security/pam_appl.h>
#include <stdlib.h>
#include <string.h>

static pam_handle_t *pam_handle;

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
                // TODO: log
                free(*res);
                return PAM_CONV_ERR;
            case PAM_TEXT_INFO:
                // TODO: log
                break;
            default:
                // TODO: throw error
                break;
        }
    }

    return PAM_SUCCESS;
}

int login(const char *username, const char *password) {
    int status;
    const char *data[2] = {username, password};
    struct pam_conv pam_conv = {conv, data};

    status = pam_start("ssdm", username, &pam_conv, &pam_handle);
    if (status != PAM_SUCCESS) return AUTH_ERROR;
    status = pam_authenticate(pam_handle, PAM_DISALLOW_NULL_AUTHTOK);
    if (status == PAM_AUTH_ERR) {
        pam_end(pam_handle, status);
        return AUTH_WRONG_CREDENTIALS;
    }
    if (status != PAM_SUCCESS) goto on_error;
    status = pam_acct_mgmt(pam_handle, PAM_DISALLOW_NULL_AUTHTOK);
    if (status != PAM_SUCCESS) goto on_error;
    status = pam_setcred(pam_handle, PAM_ESTABLISH_CRED);
    if (status != PAM_SUCCESS) goto on_error;
    status = pam_open_session(pam_handle, 0);
    if (status != PAM_SUCCESS) goto on_error;

    return AUTH_SUCCESS;
on_error:
    pam_end(pam_handle, status);
    return AUTH_ERROR;
}

int logout(void) {
    int status;

    status = pam_close_session(pam_handle, 0);
    if (status != PAM_SUCCESS) goto on_error;
    status = pam_setcred(pam_handle, PAM_DELETE_CRED);
    if (status != PAM_SUCCESS) goto on_error;
    pam_end(pam_handle, status);

    return AUTH_SUCCESS;
on_error:
    pam_end(pam_handle, status);
    return AUTH_ERROR;
}