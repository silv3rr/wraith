#ifndef _BOTCMD_H
#define _BOTCMD_H

void bounce_simul(int, char *);
void send_remote_simul(int, char *, char *, char *);
void bot_share(int, char *);
void init_botcmd(void);
void parse_botcmd(int, const char*, char*);

#endif /* !_BOTCMD_H */
