#ifndef _USERREC_H
#define _USERREC_H

namespace bd {
  class Stream;
}

struct userrec *adduser(struct userrec *, char *, char *, char *, flag_t, int);
void addhost_by_handle(char *, char *);
void clear_masks(struct maskrec *);
void clear_userlist(struct userrec *);
int u_pass_match(struct userrec *, char *);
int delhost_by_handle(char *, char *);
int count_users(struct userrec *);
int deluser(char *);
int change_handle(struct userrec *, char *);
void correct_handle(char *);
void stream_writeuserfile(bd::Stream&, const struct userrec *, int, bool = 0);
int write_userfile(int);
int real_writeuserfile(int idx, const struct userrec *bu, FILE *f, bool = 0);
void touch_laston(struct userrec *, char *, time_t);
void user_del_chan(char *);
struct userrec *host_conflicts(char *);

extern struct userrec  		*userlist, *lastuser;
extern int			cache_hit, cache_miss, userfile_perm;
extern bool			noshare;
#endif /* !_USERREC_H */
