#include <sys/stat.h>
#include "main.h"
#include <errno.h>
#include "chan.h"
#include "modules.h"
#include "tandem.h"
extern struct dcc_t *dcc;
extern int dcc_total, max_dcc, dcc_flood_thr, backgrd, MAXSOCKS;
extern char botnetnick[], version[];
extern time_t now;
extern sock_list *socklist;
extern Tcl_Interp *interp;
char motdfile[121] = "text/motd";
int connect_timeout = 15;
int reserved_port_min = 0;
int reserved_port_max = 0;
void
init_dcc_max ()
{
  int osock = MAXSOCKS;
  if (max_dcc < 1)
    max_dcc = 1;
  if (dcc)
    dcc = nrealloc (dcc, sizeof (struct dcc_t) * max_dcc);
  else
    dcc = nmalloc (sizeof (struct dcc_t) * max_dcc);
  MAXSOCKS = max_dcc + 10;
  if (socklist)
    socklist =
      (sock_list *) nrealloc ((void *) socklist,
			      sizeof (sock_list) * MAXSOCKS);
  else
    socklist = (sock_list *) nmalloc (sizeof (sock_list) * MAXSOCKS);
  for (; osock < MAXSOCKS; osock++)
    socklist[osock].flags = SOCK_UNUSED;
}

int
expmem_dccutil ()
{
  int tot, i;
  tot = sizeof (struct dcc_t) * max_dcc + sizeof (sock_list) * MAXSOCKS;
  for (i = 0; i < dcc_total; i++)
    {
      if (dcc[i].type && dcc[i].type->expmem)
	tot += dcc[i].type->expmem (dcc[i].u.other);
    }
  return tot;
}

char *
add_cr (char *buf)
{
  static char WBUF[1024];
  char *p, *q;
  for (p = buf, q = WBUF; *p; p++, q++)
    {
      if (*p == '\n')
	*q++ = '\r';
      *q = *p;
    }
  *q = *p;
  return WBUF;
}
extern void (*qserver) (int, char *, int);
void dprintf
EGG_VARARGS_DEF (int, arg1)
{
  static char buf[1024];
  char *format, buf3[1024] = "", buf2[1024] = "", c;
  int idx, len, id;
  va_list va;
  idx = EGG_VARARGS_START (int, arg1, va);
  format = va_arg (va, char *);
  egg_vsnprintf (buf, 1023, format, va);
  va_end (va);
  buf[sizeof (buf) - 1] = 0;
  len = strlen (buf);
  id = idx;
  if (id < 0)
    {
      id = idx + 7;
      id = -id;
    }
  if ((id < 0x7FF0) && (dcc[id].status & STAT_COLOR)
      && (dcc[id].type == &DCC_CHAT))
    {
      int i, a = 0, m = 0;
      if (dcc[id].status & STAT_COLORM)
	m = 1;
      else if (dcc[id].status & STAT_COLORA)
	a = 1;
      if (!a && !m)
	{
	  goto broke;
	}
      buf3[0] = '\0';
      for (i = 0; i < len; i++)
	{
	  c = buf[i];
	  buf2[0] = '\0';
	  if (c == ':')
	    {
	      if (a)
		sprintf (buf2, "\e[%d;%dm%c\e[0m", 0, 37, c);
	      else
		sprintf (buf2, "\003%d%c\003\002\002", 15, c);
	    }
	  else if (c == '@')
	    {
	      if (a)
		sprintf (buf2, "\e[1m%c\e[0m", c);
	      else
		sprintf (buf2, "\002%c\002", c);
	    }
	  else if (c == ']' || c == '>' || c == ')')
	    {
	      if (a)
		sprintf (buf2, "\e[%d;%dm%c\e[0m", 0, 32, c);
	      else
		sprintf (buf2, "\00303%c\003\002\002", c);
	    }
	  else if (c == '[' || c == '<' || c == '(')
	    {
	      if (a)
		sprintf (buf2, "\e[%d;%dm%c\e[0m", 0, 32, c);
	      else
		sprintf (buf2, "\00303%c\003\002\002", c);
	    }
	  else
	    {
	      sprintf (buf2, "%c", c);
	    }
	  sprintf (buf3, "%s%s", buf3 ? buf3 : "", buf2 ? buf2 : "");
	}
      buf3[strlen (buf3)] = '\0';
      strcpy (buf, buf3);
    }
broke:buf[sizeof (buf) - 1] = 0;
  len = strlen (buf);
  if (idx < 0)
    {
      tputs (-idx, buf, len);
    }
  else if (idx > 0x7FF0)
    {
      switch (idx)
	{
	case DP_LOG:
	  putlog (LOG_MISC, "*", "%s", buf);
	  break;
	case DP_STDOUT:
	  tputs (STDOUT, buf, len);
	  break;
	case DP_STDERR:
	  tputs (STDERR, buf, len);
	  break;
	case DP_SERVER:
#ifdef HUB
	  return;
#endif
	case DP_HELP:
#ifdef HUB
	  return;
#endif
	case DP_MODE:
#ifdef HUB
	  return;
#endif
	case DP_MODE_NEXT:
#ifdef HUB
	  return;
#endif
	case DP_SERVER_NEXT:
#ifdef HUB
	  return;
#endif
	case DP_HELP_NEXT:
#ifdef HUB
	  return;
#endif
	  qserver (idx, buf, len);
	  break;
	}
      return;
    }
  else
    {
      if (len > 500)
	{
	  buf[500] = 0;
	  strcat (buf, "\n");
	  len = 501;
	}
      if (dcc[idx].type && ((long) (dcc[idx].type->output) == 1))
	{
	  char *p = add_cr (buf);
	  tputs (dcc[idx].sock, p, strlen (p));
	}
      else if (dcc[idx].type && dcc[idx].type->output)
	{
	  dcc[idx].type->output (idx, buf, dcc[idx].u.other);
	}
      else
	tputs (dcc[idx].sock, buf, len);
    }
}
void chatout
EGG_VARARGS_DEF (char *, arg1)
{
  int i, len;
  char *format;
  char s[601];
  va_list va;
  format = EGG_VARARGS_START (char *, arg1, va);
  egg_vsnprintf (s, 511, format, va);
  va_end (va);
  len = strlen (s);
  if (len > 511)
    len = 511;
  s[len + 1] = 0;
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_CHAT)
      if (dcc[i].u.chat->channel >= 0)
	dprintf (i, "%s", s);
}
void chanout_but
EGG_VARARGS_DEF (int, arg1)
{
  int i, x, chan, len;
  char *format;
  char s[601];
  va_list va;
  x = EGG_VARARGS_START (int, arg1, va);
  chan = va_arg (va, int);
  format = va_arg (va, char *);
  egg_vsnprintf (s, 511, format, va);
  va_end (va);
  len = strlen (s);
  if (len > 511)
    len = 511;
  s[len + 1] = 0;
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_CHAT) && (i != x))
      if (dcc[i].u.chat->channel == chan)
	dprintf (i, "%s", s);
}

void
dcc_chatter (int idx)
{
  int i, j;
  struct flag_record fr = { FR_GLOBAL | FR_CHAN | FR_ANYWH, 0, 0, 0, 0, 0 };
  get_user_flagrec (dcc[idx].user, &fr, NULL);
  show_motd (idx);
  if (glob_party (fr))
    {
      i = dcc[idx].u.chat->channel;
    }
  else
    {
      dprintf (idx,
	       "You don't have partyline chat access; commands only.\n\n");
      i = -1;
    }
  dcc[idx].u.chat->channel = 234567;
  j = dcc[idx].sock;
  strcpy (dcc[idx].u.chat->con_chan, "***");
  check_tcl_chon (dcc[idx].nick, dcc[idx].sock);
  if ((idx >= dcc_total) || (dcc[idx].sock != j))
    return;
  if (dcc[idx].type == &DCC_CHAT)
    {
      if (!strcmp (dcc[idx].u.chat->con_chan, "***"))
	strcpy (dcc[idx].u.chat->con_chan, "*");
      if (dcc[idx].u.chat->channel == 234567)
	{
	  if (i == -2)
	    i = 0;
	  dcc[idx].u.chat->channel = i;
	  if (dcc[idx].u.chat->channel >= 0)
	    {
	      if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
		{
		  botnet_send_join_idx (idx, -1);
		}
	    }
	  check_tcl_chjn (botnetnick, dcc[idx].nick, dcc[idx].u.chat->channel,
			  geticon (idx), dcc[idx].sock, dcc[idx].host);
	}
      if (!dcc[idx].u.chat->channel)
	{
	  chanout_but (-1, 0, "*** %s joined the party line.\n",
		       dcc[idx].nick);
	}
      else if (dcc[idx].u.chat->channel > 0)
	{
	  chanout_but (-1, dcc[idx].u.chat->channel,
		       "*** %s joined the channel.\n", dcc[idx].nick);
	}
    }
}
void
lostdcc (int n)
{
  if (n < 0 || n >= max_dcc)
    return;
  if (dcc[n].type && dcc[n].type->kill)
    dcc[n].type->kill (n, dcc[n].u.other);
  else if (dcc[n].u.other)
    nfree (dcc[n].u.other);
  egg_bzero (&dcc[n], sizeof (struct dcc_t));
  dcc[n].sock = (-1);
  dcc[n].type = &DCC_LOST;
} void
removedcc (int n)
{
  if (dcc[n].type && dcc[n].type->kill)
    dcc[n].type->kill (n, dcc[n].u.other);
  else if (dcc[n].u.other)
    nfree (dcc[n].u.other);
  dcc_total--;
  if (n < dcc_total)
    egg_memcpy (&dcc[n], &dcc[dcc_total], sizeof (struct dcc_t));
  else
    egg_bzero (&dcc[n], sizeof (struct dcc_t));
} void
dcc_remove_lost (void)
{
  int i;
  for (i = 0; i < dcc_total; i++)
    {
      if (dcc[i].type == &DCC_LOST)
	{
	  dcc[i].type = NULL;
	  dcc[i].sock = (-1);
	  removedcc (i);
	  i--;
	}
    }
}
void
tell_dcc (int zidx)
{
  int i, j;
  char other[160];
  char format[81];
  int nicklen;
  nicklen = 0;
  for (i = 0; i < dcc_total; i++)
    {
      if (strlen (dcc[i].nick) > nicklen)
	nicklen = strlen (dcc[i].nick);
    }
  if (nicklen < 9)
    nicklen = 9;
  egg_snprintf (format, sizeof format,
		"%%-4s %%-8s %%-5s %%-%us %%-17s %%s\n", nicklen);
  dprintf (zidx, format, "SOCK", "ADDR", "PORT", "NICK", "HOST", "TYPE");
  dprintf (zidx, format, "----", "--------", "-----", "---------",
	   "-----------------", "----");
  egg_snprintf (format, sizeof format, "%%-4d %%08X %%5d %%-%us %%-17s %%s\n",
		nicklen);
  for (i = 0; i < dcc_total; i++)
    {
      j = strlen (dcc[i].host);
      if (j > 17)
	j -= 17;
      else
	j = 0;
      if (dcc[i].type && dcc[i].type->display)
	dcc[i].type->display (i, other);
      else
	{
	  sprintf (other, "?:%lX  !! ERROR !!", (long) dcc[i].type);
	  break;
	}
      dprintf (zidx, format, dcc[i].sock, dcc[i].addr, dcc[i].port,
	       dcc[i].nick, dcc[i].host + j, other);
    }
}
void
not_away (int idx)
{
  if (dcc[idx].u.chat->away == NULL)
    {
      dprintf (idx, "You weren't away!\n");
      return;
    }
  if (dcc[idx].u.chat->channel >= 0)
    {
      chanout_but (-1, dcc[idx].u.chat->channel,
		   "*** %s is no longer away.\n", dcc[idx].nick);
      if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
	{
	  botnet_send_away (-1, botnetnick, dcc[idx].sock, NULL, idx);
	}
    }
  dprintf (idx, "You're not away any more.\n");
  nfree (dcc[idx].u.chat->away);
  dcc[idx].u.chat->away = NULL;
  check_tcl_away (botnetnick, dcc[idx].sock, NULL);
}

void
set_away (int idx, char *s)
{
  if (s == NULL)
    {
      not_away (idx);
      return;
    }
  if (!s[0])
    {
      not_away (idx);
      return;
    }
  if (dcc[idx].u.chat->away != NULL)
    nfree (dcc[idx].u.chat->away);
  dcc[idx].u.chat->away = (char *) nmalloc (strlen (s) + 1);
  strcpy (dcc[idx].u.chat->away, s);
  if (dcc[idx].u.chat->channel >= 0)
    {
      chanout_but (-1, dcc[idx].u.chat->channel, "*** %s is now away: %s\n",
		   dcc[idx].nick, s);
      if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
	{
	  botnet_send_away (-1, botnetnick, dcc[idx].sock, s, idx);
	}
    }
  dprintf (idx, "You are now away.\n");
  check_tcl_away (botnetnick, dcc[idx].sock, s);
}

void *
_get_data_ptr (int size, char *file, int line)
{
  char *p;
#ifdef DEBUG_MEM
  char x[1024];
  p = strrchr (file, '/');
  egg_snprintf (x, sizeof x, "dccutil.c:%s", p ? p + 1 : file);
  p = n_malloc (size, x, line);
#else
  p = nmalloc (size);
#endif
  egg_bzero (p, size);
  return p;
}

void
makepass (char *s)
{
  int i;
  i = 10 + (random () % 6);
  make_rand_str (s, i);
} void
flush_lines (int idx, struct chat_info *ci)
{
  int c = ci->line_count;
  struct msgq *p = ci->buffer, *o;
  while (p && c < (ci->max_line))
    {
      ci->current_lines--;
      tputs (dcc[idx].sock, p->msg, p->len);
      nfree (p->msg);
      o = p->next;
      nfree (p);
      p = o;
      c++;
    }
  if (p != NULL)
    {
      if (dcc[idx].status & STAT_TELNET)
	tputs (dcc[idx].sock, "[More]: ", 8);
      else
	tputs (dcc[idx].sock, "[More]\n", 7);
    }
  ci->buffer = p;
  ci->line_count = 0;
}

int
new_dcc (struct dcc_table *type, int xtra_size)
{
  int i = dcc_total;
  if (dcc_total == max_dcc)
    return -1;
  dcc_total++;
  egg_bzero ((char *) &dcc[i], sizeof (struct dcc_t));
  dcc[i].type = type;
  if (xtra_size)
    {
      dcc[i].u.other = nmalloc (xtra_size);
      egg_bzero (dcc[i].u.other, xtra_size);
    }
  return i;
}

void
changeover_dcc (int i, struct dcc_table *type, int xtra_size)
{
  if (dcc[i].type && dcc[i].type->kill)
    dcc[i].type->kill (i, dcc[i].u.other);
  else if (dcc[i].u.other)
    {
      nfree (dcc[i].u.other);
      dcc[i].u.other = NULL;
    }
  dcc[i].type = type;
  if (xtra_size)
    {
      dcc[i].u.other = nmalloc (xtra_size);
      egg_bzero (dcc[i].u.other, xtra_size);
    }
}
int
detect_dcc_flood (time_t * timer, struct chat_info *chat, int idx)
{
  time_t t;
  if (!dcc_flood_thr)
    return 0;
  t = now;
  if (*timer != t)
    {
      *timer = t;
      chat->msgs_per_sec = 0;
    }
  else
    {
      chat->msgs_per_sec++;
      if (chat->msgs_per_sec > dcc_flood_thr)
	{
	  dprintf (idx, "*** FLOOD: %s.\n", IRC_GOODBYE);
	  if ((dcc[idx].type->flags & DCT_CHAT) && chat
	      && (chat->channel >= 0))
	    {
	      char x[1024];
	      egg_snprintf (x, sizeof x, DCC_FLOODBOOT, dcc[idx].nick);
	      chanout_but (idx, chat->channel, "*** %s", x);
	      if (chat->channel < GLOBAL_CHANS)
		botnet_send_part_idx (idx, x);
	    }
	  check_tcl_chof (dcc[idx].nick, dcc[idx].sock);
	  if ((dcc[idx].sock != STDOUT) || backgrd)
	    {
	      killsock (dcc[idx].sock);
	      lostdcc (idx);
	    }
	  else
	    {
	      dprintf (DP_STDOUT, "\n### SIMULATION RESET ###\n\n");
	      dcc_chatter (idx);
	    }
	  return 1;
	}
    }
  return 0;
}

void
do_boot (int idx, char *by, char *reason)
{
  int files = (dcc[idx].type != &DCC_CHAT);
  dprintf (idx, DCC_BOOTED1);
  dprintf (idx, DCC_BOOTED2, files ? "file section" : "bot", by,
	   reason[0] ? ": " : ".", reason);
  if ((dcc[idx].type->flags & DCT_CHAT) && (dcc[idx].u.chat->channel >= 0))
    {
      char x[1024];
      egg_snprintf (x, sizeof x, DCC_BOOTED3, by, dcc[idx].nick,
		    reason[0] ? ": " : "", reason);
      chanout_but (idx, dcc[idx].u.chat->channel, "*** %s.\n", x);
      if (dcc[idx].u.chat->channel < GLOBAL_CHANS)
	botnet_send_part_idx (idx, x);
    }
  check_tcl_chof (dcc[idx].nick, dcc[idx].sock);
  if ((dcc[idx].sock != STDOUT) || backgrd)
    {
      killsock (dcc[idx].sock);
      lostdcc (idx);
    }
  else
    {
      dprintf (DP_STDOUT, "\n### SIMULATION RESET\n\n");
      dcc_chatter (idx);
    }
  return;
}
