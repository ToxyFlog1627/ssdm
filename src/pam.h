#ifndef PAM_H
#define PAM_H

#define AUTH_SUCCESS 0
#define AUTH_WRONG_CREDENTIALS 1
#define AUTH_ERROR 2

int pam_login(const char *username, const char *password);
int pam_logout(void);
void pam_init_env(void);

#endif  // PAM_H