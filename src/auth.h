#ifndef _AUTH_H
#  define _AUTH_H

#  include "crypt.h"
#include <bdlib/src/String.h>
#include <bdlib/src/HashTable.h>

class RfcString;

#define AUTHED	    1
#define AUTHING     2
/* These are what we are expecting back from the user */
#define AUTH_PASS   3
#define AUTH_HASH   4
#define AUTH_BDHASH 5

class Auth {
  public:
  Auth(const RfcString&, const char *, struct userrec * = NULL);
  ~Auth();

  inline int Status(void) const noexcept __attribute__((pure)) {
    return status;
  }
  int Status(int newstat) noexcept {
    status = newstat;
    return Status();
  }
  void MakeHash() noexcept;
  bool Authed() const noexcept __attribute__((pure)) {
    return (status == AUTHED);
  }
  bool GetIdx(const char *) noexcept;
  void Done() noexcept;
  void NewNick(const RfcString&) noexcept;

  static Auth *Find(const char * host) noexcept __attribute__((pure));
  static void NullUsers(const RfcString&) noexcept;
  static void NullUsers(void) noexcept;
  static void FillUsers(void) noexcept;
  static void ExpireAuths() noexcept;
  static void InitTimer() noexcept;
  static void DeleteAll() noexcept;
  static void TellAuthed(int) noexcept;

  struct userrec *user;
  time_t authtime;              /* what time they authed at */
  time_t atime;                 /* when they last were active */
  int idx;			/* do they have an associated idx? */
  char hash[MD5_HASH_LENGTH + 1];       /* used for dcc authing */
  char rand[51];
  RfcString nick;
  char host[UHOSTLEN];

  static bd::HashTable<bd::String, Auth*> ht_host;
  static bd::HashTable<RfcString, Auth*> ht_nick;

  private:
  int status;
};

void makehash(struct userrec *u, const char *randstring, char *out, size_t out_size);

int check_auth_dcc(Auth *, const char *, const char *);

#endif /* !_AUTH_H */
