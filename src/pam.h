#ifndef PAM_H
#define PAM_H

#define AUTH_SUCCESS 0
#define AUTH_ERROR 1
#define AUTH_WRONG_CREDENTIALS 2
#define AUTH_TOO_MANY_ATTEMPTS 3

void pam_init_env(void);
int pam_login(const char *username, const char *password);
int pam_logout(void);

#endif  // PAM_H