/*
 * tcldcc.c -- handles:
 *   Tcl stubs for the dcc commands
 *
 */

#include "main.h"
#include "tandem.h"
#include "modules.h"

#include "blowfish_conf.h"
#include <sys/stat.h>

extern Tcl_Interp	*interp;
extern tcl_timer_t	*timer,
			*utimer;
extern struct dcc_t	*dcc;

extern int		 dcc_total, backgrd, parties,
			 do_restart, remote_boots, max_dcc, hub, leaf;

extern char		 botnetnick[], netpass[], *binname;
extern party_t		*party;
extern tand_t		*tandbot;
extern time_t		 now;

/* Traffic stuff. */
extern unsigned long otraffic_irc, otraffic_irc_today, itraffic_irc, itraffic_irc_today, otraffic_bn, otraffic_bn_today, itraffic_bn, itraffic_bn_today, otraffic_dcc, otraffic_dcc_today, itraffic_dcc, itraffic_dcc_today, otraffic_trans, otraffic_trans_today, itraffic_trans, itraffic_trans_today, otraffic_unknown, otraffic_unknown_today, itraffic_unknown, itraffic_unknown_today;

int			 enable_simul = 0;
static struct portmap	*root = NULL;


int expmem_tcldcc(void)
{
  int tot = 0;
  struct portmap *pmap;

  for (pmap = root; pmap; pmap = pmap->next)
    tot += sizeof(struct portmap);

  return tot;
}

/***********************************************************************/

static int tcl_putdcc STDVAR
{
  int i, j;

  BADARGS(3, 3, " idx text");
  i = atoi(argv[1]);
  j = findidx(i);
  if (j < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  dumplots(-i, "", argv[2]);
  return TCL_OK;
}

/* Allows tcl scripts to send out raw data. Can be used for fast server
 * write (idx=0)
 *
 * usage:
 * 	putdccraw <idx> <size> <rawdata>
 * example:
 * 	putdccraw 6 13 "eggdrop rulz\n"
 *
 * (added by drummer@sophia.jpte.hu)
 */

static int tcl_putdccraw STDVAR
{
  int i, j = 0, z;

  BADARGS(4, 4, " idx size text");
  z = atoi(argv[1]);
  for (i = 0; i < dcc_total; i++) {
    if (!z && !strcmp(dcc[i].nick, "(server)")) {
      j = dcc[i].sock;
      break;
    } else if (dcc[i].sock == z) {
      j = dcc[i].sock;
      break;
    }
  }
  if (i == dcc_total) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  tputs(j, argv[3], atoi(argv[2]));
  return TCL_OK;
}

static int tcl_dccsimul STDVAR
{
  BADARGS(3, 3, " idx command");
  if (enable_simul) {
    int idx = findidx(atoi(argv[1]));

    if (idx >= 0 && (dcc[idx].type->flags & DCT_SIMUL)) {
      int l = strlen(argv[2]);

      if (l > 510) {
	l = 510;
	argv[2][510] = 0;	/* Restrict length of cmd */
      }
      if (dcc[idx].type && dcc[idx].type->activity) {
	dcc[idx].type->activity(idx, argv[2], l);
	return TCL_OK;
      }
    } else
      Tcl_AppendResult(irp, "invalid idx", NULL);
  } else
    Tcl_AppendResult(irp, "simul disabled", NULL);
  return TCL_ERROR;
}

static int tcl_dccbroadcast STDVAR
{
  char msg[sgrab-110];

  BADARGS(2, 2, " message");
  strncpyz(msg, argv[1], sizeof msg);
  chatout("*** %s\n", msg);
  botnet_send_chat(-1, botnetnick, msg);
  return TCL_OK;
}

static int tcl_hand2idx STDVAR
{
  int i;
  char s[11];

  BADARGS(2, 2, " nickname");
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type->flags & DCT_SIMUL) &&
        !egg_strcasecmp(argv[1], dcc[i].nick)) {
      egg_snprintf(s, sizeof s, "%ld", dcc[i].sock);
      Tcl_AppendResult(irp, s, NULL);
      return TCL_OK;
    }
  Tcl_AppendResult(irp, "-1", NULL);
  return TCL_OK;
}

static int tcl_getchan STDVAR
{
  char s[7];
  int idx;

  BADARGS(2, 2, " idx");
  idx = findidx(atoi(argv[1]));
  if (idx < 0 ||
      (dcc[idx].type != &DCC_CHAT && dcc[idx].type != &DCC_SCRIPT)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].type == &DCC_SCRIPT)
    egg_snprintf(s, sizeof s, "%d", dcc[idx].u.script->u.chat->channel);
  else
    egg_snprintf(s, sizeof s, "%d", dcc[idx].u.chat->channel);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_setchan STDVAR
{
  int idx, chan;
  module_entry *me;

  BADARGS(3, 3, " idx channel");
  idx = findidx(atoi(argv[1]));
  if (idx < 0 ||
      (dcc[idx].type != &DCC_CHAT && dcc[idx].type != &DCC_SCRIPT)) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (argv[2][0] < '0' || argv[2][0] > '9') {
    if (!strcmp(argv[2], "-1") || !egg_strcasecmp(argv[2], "off"))
      chan = (-1);
    else {
      Tcl_SetVar(irp, "chan", argv[2], 0);
      if (Tcl_VarEval(irp, "assoc ", "$chan", NULL) != TCL_OK ||
	  !interp->result[0]) {
	Tcl_AppendResult(irp, "channel name is invalid", NULL);
	return TCL_ERROR;
      }
      chan = atoi(interp->result);
    }
  } else
    chan = atoi(argv[2]);
  if ((chan < -1) || (chan > 199999)) {
    Tcl_AppendResult(irp, "channel out of range; must be -1 thru 199999",
		     NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].type == &DCC_SCRIPT)
    dcc[idx].u.script->u.chat->channel = chan;
  else {
    int oldchan = dcc[idx].u.chat->channel;

    if (dcc[idx].u.chat->channel >= 0) {
      if ((chan >= GLOBAL_CHANS) && (oldchan < GLOBAL_CHANS))
	botnet_send_part_idx(idx, "*script*");
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
		     dcc[idx].u.chat->channel);
    }
    dcc[idx].u.chat->channel = chan;
    if (chan < GLOBAL_CHANS)
      botnet_send_join_idx(idx, oldchan);
    check_tcl_chjn(botnetnick, dcc[idx].nick, chan, geticon(idx),
		   dcc[idx].sock, dcc[idx].host);
  }
  /* Console autosave. */
  if ((me = module_find("console", 0, 0))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (idx);
  }
  return TCL_OK;
}

static int tcl_dccputchan STDVAR
{
  int chan;
  char msg[sgrab-110];

  BADARGS(3, 3, " channel message");
  chan = atoi(argv[1]);
  if ((chan < 0) || (chan > 199999)) {
    Tcl_AppendResult(irp, "channel out of range; must be 0 thru 199999",
		     NULL);
    return TCL_ERROR;
  }
  strncpyz(msg, argv[2], sizeof msg);
  chanout_but(-1, chan, "*** %s\n", argv[2]);
  botnet_send_chan(-1, botnetnick, NULL, chan, argv[2]);
  check_tcl_bcst(botnetnick, chan, argv[2]);
  return TCL_OK;
}

static int tcl_console STDVAR
{
  int i, j, pls, arg;
  module_entry *me;

  BADARGS(2, 4, " idx ?channel? ?console-modes?");
  i = findidx(atoi(argv[1]));
  if (i < 0 || dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  pls = 1;
  for (arg = 2; arg < argc; arg++) {
    if (argv[arg][0] && ((strchr(CHANMETA, argv[arg][0]) != NULL) ||
	(argv[arg][0] == '*'))) {
      if ((argv[arg][0] != '*') && (!findchan_by_dname(argv[arg]))) {
        /* If we dont find the channel, and it starts with a +... assume it
         * should be the console flags to set.
         */
        if (argv[arg][0]=='+')
          goto do_console_flags;
	Tcl_AppendResult(irp, "invalid channel", NULL);
	return TCL_ERROR;
      }
      strncpyz(dcc[i].u.chat->con_chan, argv[arg], 81);
    } else {
      if ((argv[arg][0] != '+') && (argv[arg][0] != '-'))
	dcc[i].u.chat->con_flags = 0;
      do_console_flags:
      for (j = 0; j < strlen(argv[arg]); j++) {
	if (argv[arg][j] == '+')
	  pls = 1;
	else if (argv[arg][j] == '-')
	  pls = (-1);
	else {
	  char s[2];

	  s[0] = argv[arg][j];
	  s[1] = 0;
	  if (pls == 1)
	    dcc[i].u.chat->con_flags |= logmodes(s);
	  else
	    dcc[i].u.chat->con_flags &= ~logmodes(s);
	}
      }
    }
  }
  Tcl_AppendElement(irp, dcc[i].u.chat->con_chan);
  Tcl_AppendElement(irp, masktype(dcc[i].u.chat->con_flags));
  /* Console autosave. */
  if (argc > 2 && (me = module_find("console", 0, 0))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (i);
  }
  return TCL_OK;
}

static int tcl_strip STDVAR
{
  int i, j, pls, arg;
  module_entry *me;

  BADARGS(2, 4, " idx ?strip-flags?");
  i = findidx(atoi(argv[1]));
  if (i < 0 || dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  pls = 1;
  for (arg = 2; arg < argc; arg++) {
    if ((argv[arg][0] != '+') && (argv[arg][0] != '-'))
      dcc[i].u.chat->strip_flags = 0;
    for (j = 0; j < strlen(argv[arg]); j++) {
      if (argv[arg][j] == '+')
	pls = 1;
      else if (argv[arg][j] == '-')
	pls = (-1);
      else {
	char s[2];

	s[0] = argv[arg][j];
	s[1] = 0;
	if (pls == 1)
	  dcc[i].u.chat->strip_flags |= stripmodes(s);
	else
	  dcc[i].u.chat->strip_flags &= ~stripmodes(s);
      }
    }
  }
  Tcl_AppendElement(irp, stripmasktype(dcc[i].u.chat->strip_flags));
  /* Console autosave. */
  if (argc > 2 && (me = module_find("console", 0, 0))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (i);
  }
  return TCL_OK;
}

static int tcl_echo STDVAR
{
  int i;
  module_entry *me;

  BADARGS(2, 3, " idx ?status?");
  i = findidx(atoi(argv[1]));
  if (i < 0 || dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (argc == 3) {
    if (atoi(argv[2]))
      dcc[i].status |= STAT_ECHO;
    else
      dcc[i].status &= ~STAT_ECHO;
  }
  if (dcc[i].status & STAT_ECHO)
    Tcl_AppendResult(irp, "1", NULL);
  else
    Tcl_AppendResult(irp, "0", NULL);
  /* Console autosave. */
  if (argc > 2 && (me = module_find("console", 0, 0))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (i);
  }
  return TCL_OK;
}
static int tcl_page STDVAR
{
  int i;
  char x[20];
  module_entry *me;

  BADARGS(2, 3, " idx ?status?");
  i = findidx(atoi(argv[1]));
  if (i < 0 || dcc[i].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (argc == 3) {
    int l = atoi(argv[2]);

    if (!l)
      dcc[i].status &= ~STAT_PAGE;
    else {
      dcc[i].status |= STAT_PAGE;
      dcc[i].u.chat->max_line = l;
    }
  }
  if (dcc[i].status & STAT_PAGE) {
    egg_snprintf(x, sizeof x, "%d", dcc[i].u.chat->max_line);
    Tcl_AppendResult(irp, x, NULL);
  } else
    Tcl_AppendResult(irp, "0", NULL);
  /* Console autosave. */
  if ((argc > 2) && (me = module_find("console", 0, 0))) {
    Function *func = me->funcs;

    (func[CONSOLE_DOSTORE]) (i);
  }
  return TCL_OK;
}

static int tcl_control STDVAR
{
  int idx;
  void *hold;

  BADARGS(3, 3, " idx command");
  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].type->flags & DCT_CHAT) {
    if (dcc[idx].u.chat->channel >= 0) {
      chanout_but(idx, dcc[idx].u.chat->channel, "*** %s has gone.\n",
		  dcc[idx].nick);
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
		     dcc[idx].u.chat->channel);
      botnet_send_part_idx(idx, "gone");
    }
    check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
  }
  hold = dcc[idx].u.other;
  dcc[idx].u.script = get_data_ptr(sizeof(struct script_info));
  dcc[idx].u.script->u.other = hold;
  dcc[idx].u.script->type = dcc[idx].type;
  dcc[idx].type = &DCC_SCRIPT;
  /* Do not buffer data anymore. All received and stored data is passed
     over to the dcc functions from now on.  */
  sockoptions(dcc[idx].sock, EGG_OPTION_UNSET, SOCK_BUFFER);
  strncpyz(dcc[idx].u.script->command, argv[2], 120);
  return TCL_OK;
}

static int tcl_valididx STDVAR
{
  int idx;

  BADARGS(2, 2, " idx");
  idx = findidx(atoi(argv[1]));
  if (idx < 0 || !(dcc[idx].type->flags & DCT_VALIDIDX))
     Tcl_AppendResult(irp, "0", NULL);
  else
     Tcl_AppendResult(irp, "1", NULL);
   return TCL_OK;
}

static int tcl_killdcc STDVAR
{
  int idx;

  BADARGS(2, 3, " idx ?reason?");
  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  /* Don't kill terminal socket */
  if ((dcc[idx].sock == STDOUT) && !backgrd)
    return TCL_OK;
  /* Make sure 'whom' info is updated for other bots */
  if (dcc[idx].type->flags & DCT_CHAT) {
    chanout_but(idx, dcc[idx].u.chat->channel, "*** %s has left the %s%s%s\n",
		dcc[idx].nick, dcc[idx].u.chat ? "channel" : "partyline",
		argc == 3 ? ": " : "", argc == 3 ? argv[2] : "");
    botnet_send_part_idx(idx, argc == 3 ? argv[2] : "");
    if ((dcc[idx].u.chat->channel >= 0) &&
	(dcc[idx].u.chat->channel < GLOBAL_CHANS))
      check_tcl_chpt(botnetnick, dcc[idx].nick, dcc[idx].sock,
		     dcc[idx].u.chat->channel);
    check_tcl_chof(dcc[idx].nick, dcc[idx].sock);
    /* Notice is sent to the party line, the script can add a reason. */
  }
  killsock(dcc[idx].sock);
  lostdcc(idx);
  return TCL_OK;
}

static int tcl_putbot STDVAR
{
  int i;
  char msg[sgrab-110];

  BADARGS(3, 3, " botnick message");
  i = nextbot(argv[1]);
  if (i < 0) {
    Tcl_AppendResult(irp, "bot is not in the botnet", NULL);
    return TCL_ERROR;
  }
  strncpyz(msg, argv[2], sizeof msg);
  botnet_send_zapf(i, botnetnick, argv[1], msg);
  return TCL_OK;
}

static int tcl_putallbots STDVAR
{
  char msg[sgrab-110];

  BADARGS(2, 2, " message");
  strncpyz(msg, argv[1], sizeof msg);
  botnet_send_zapf_broad(-1, botnetnick, NULL, msg);
  return TCL_OK;
}

static int tcl_idx2hand STDVAR
{
  int idx;

  BADARGS(2, 2, " idx");
  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  Tcl_AppendResult(irp, dcc[idx].nick, NULL);
  return TCL_OK;
}

static int tcl_islinked STDVAR
{
  int i;

  BADARGS(2, 2, " bot");
  i = nextbot(argv[1]);
  if (i < 0)
     Tcl_AppendResult(irp, "0", NULL);
  else
     Tcl_AppendResult(irp, "1", NULL);
   return TCL_OK;
}
static int tcl_randstring STDVAR
{
 int length = atoi(argv[1]);
 char s[length+1];

 BADARGS(2, 2, " length");
 if (length) {
  make_rand_str(s,length);
  Tcl_AppendResult(irp, s, NULL);
 } else
  Tcl_AppendResult(irp, "", NULL);
 return TCL_OK;
}
//This function is simply for the tcl to check if the right binary is being used.
static int tcl_hcheck STDVAR
{
  Tcl_AppendResult(irp, "1", NULL);
  return TCL_OK;
}
static int tcl_binname STDVAR
{
 Tcl_AppendResult(irp, binname, NULL);
 return TCL_OK;
}
static int tcl_configend STDVAR 
{
 Tcl_AppendResult(irp, "1", NULL);
 return TCL_OK;
}
static int tcl_esource STDVAR
{

  int code, nc, skip = 0, incom = 0, lines = 0, line = 0;

  FILE  *f;
  char *buf, *tptr, templine[2048];
  char *temps = NULL, temps2[2048];
  char *horeting, check[2048];


  struct stat st;

  BADARGS(2, 2, " file");

  /* Check whether file is readable. */
  if ((f = fopen(argv[1], "r")) == NULL)
    return 0;
  fclose(f);

  if (stat(argv[1], &st)) {
    fatal("broken file", 0);
  }

  buf = nmalloc(st.st_size * 2.5);
  *buf = 0;

  f = fopen(argv[1], "r");
  if (!f)
    fatal("broken file", 0);

  while(fgets(templine, sizeof(templine), f)) {
    nc = 0;

    if(strchr(templine, '\n')) {
      tptr = templine;
      while( (tptr = strchr(tptr, '\n')) ) {
        nc++;
        *tptr++ = 0;
      }
    }

    horeting = decryptit(templine);
    temps = (char *) decrypt_string(netpass, horeting);
    temps2[0] = '\0';
    strcpy(temps2,temps);
    nfree(temps);
    strcpy(check,temps2);
    line++;
    if (!strncmp(check,"-ifhub",6) || !strncmp(check,"-ifleaf",7) || !strncmp(check,"-endif",6) || (!strncmp(check,"-else",5) && (skip == 1 || skip == 2)) || !strncmp(check,"/*",2) || !strncmp(check,"*/",2) || !strcmp(check+(strlen(check)-2),"*/") || !strncmp(check,"//",2)) {
      /* skip == 1 means hub code is being processed, 2 is leaf code */
     if (!strncmp(check,"-",1) && !incom) { //check for changing defines, but not if in comment.
      if (!strncmp(check,"-ifhub",6)) 
       skip = 1;
      else if (!strncmp(check,"-ifleaf",7))
       skip = 2;
      else if (!strncmp(check,"-endif",6)) { //end the ifdef
       if (skip)
        skip = 0;
       else {
        putlog(LOG_MISC, "*", "Error on line %i -endif with no ifdef!", line);
        return 0;
       }
      } 
      else if (!strncmp(check,"-else",5)) { //change the ifdef from hub->leaf or leaf->hub
       if (skip == 1) 
	skip = 2;
       else if (skip == 2)
	skip = 1;
       else {
        putlog(LOG_MISC, "*", "Error on line %i -else with no ifdef!", line);
        return 0;
       }
      }
     } 
     else { //then it must be a comment!
      if (!strncmp(check,"/*",2))
       if (!incom)
	incom = 1;
       else {
        putlog(LOG_MISC, "*", "Error on line %i New /* inside a /*", line);
        return 0;
       }
      else if (!strcmp(check+(strlen(check)-2),"*/") || !strncmp(check,"*/",2)) {
       if (incom)
        incom = 0;
       else {
        putlog(LOG_MISC, "*", "Error on line %i */ without a /*", line);
        return 0;
       }
      }
      //we dont need to check for // because its a 1 line skip, the actual read is an ELSE.
     }
     check[0] = '\0';
    }
    else {
     if ((((hub && (skip == 1)) || (leaf && (skip == 2))) || (!skip)) && !incom) {
      lines++;
      strcat(buf, temps2);
      while (nc > 0) {
        strcat(buf, "\n");
        nc--;
      }
     }
    }
  }
//  putlog(LOG_MISC, "@", "Loading %i lines from tcl", lines);
  code = Tcl_Eval(interp, buf);
  memset(buf, 0, st.st_size*2.5);
  nfree(buf);
  return code;
}

static int tcl_bots STDVAR
{
  tand_t *bot;

  BADARGS(1, 1, "");
  for (bot = tandbot; bot; bot = bot->next)
     Tcl_AppendElement(irp, bot->bot);
   return TCL_OK;
}

static int tcl_botlist STDVAR
{
  tand_t *bot;
  char *p;
  char sh[2], string[20];
#if (((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)) || (TCL_MAJOR_VERSION > 8))
    CONST char *list[4];
#else
    char *list[4];
#endif

  BADARGS(1, 1, "");
  sh[1] = 0;
  list[3] = sh;
  list[2] = string;
  for (bot = tandbot; bot; bot = bot->next) {
    list[0] = bot->bot;
    list[1] = (bot->uplink == (tand_t *) 1) ? botnetnick : bot->uplink->bot;
    strncpyz(string, int_to_base10(bot->ver), sizeof string);
    sh[0] = bot->share;
    p = Tcl_Merge(4, list);
    Tcl_AppendElement(irp, p);
    Tcl_Free((char *) p);
  }
  return TCL_OK;
}

/* list of { idx nick host type {other}  timestamp}
 */
static int tcl_dcclist STDVAR
{
  int i;
  char idxstr[10];
  char timestamp[11];
  char *p;
  char other[160];
#if (((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)) || (TCL_MAJOR_VERSION > 8))
    CONST char *list[6];
#else
    char *list[6];
#endif

  BADARGS(1, 2, " ?type?");
  for (i = 0; i < dcc_total; i++) {
    if (argc == 1 ||
	((argc == 2) && (dcc[i].type && !egg_strcasecmp(dcc[i].type->name, argv[1])))) {
      egg_snprintf(idxstr, sizeof idxstr, "%ld", dcc[i].sock);
      egg_snprintf(timestamp, sizeof timestamp, "%ld", dcc[i].timeval);
      if (dcc[i].type && dcc[i].type->display)
	dcc[i].type->display(i, other);
      else {
	egg_snprintf(other, sizeof other, "?:%lX  !! ERROR !!",
		     (long) dcc[i].type);
	break;
      }
      list[0] = idxstr;
      list[1] = dcc[i].nick;
      list[2] = dcc[i].host;
      list[3] = dcc[i].type ? dcc[i].type->name : "*UNKNOWN*";
      list[4] = other;
      list[5] = timestamp;
      p = Tcl_Merge(6, list);
      Tcl_AppendElement(irp, p);
      Tcl_Free((char *) p);
    }
  }
  return TCL_OK;
}

/* list of { nick bot host flag idletime awaymsg [channel]}
 */
static int tcl_whom STDVAR
{
  char c[2], idle[11], work[20], *p;
  int chan, i;
#if (((TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4)) || (TCL_MAJOR_VERSION > 8))
    CONST char *list[7];
#else
    char *list[7];
#endif

  BADARGS(2, 2, " chan");
  if (argv[1][0] == '*')
     chan = -1;
  else {
    if ((argv[1][0] < '0') || (argv[1][0] > '9')) {
      Tcl_SetVar(interp, "chan", argv[1], 0);
      if ((Tcl_VarEval(interp, "assoc ", "$chan", NULL) != TCL_OK) ||
	  !interp->result[0]) {
	Tcl_AppendResult(irp, "channel name is invalid", NULL);
	return TCL_ERROR;
      }
      chan = atoi(interp->result);
    } else
      chan = atoi(argv[1]);
    if ((chan < 0) || (chan > 199999)) {
      Tcl_AppendResult(irp, "channel out of range; must be 0 thru 199999",
		       NULL);
      return TCL_ERROR;
    }
  }
  for (i = 0; i < dcc_total; i++)
    if (dcc[i].type == &DCC_CHAT) {
      if (dcc[i].u.chat->channel == chan || chan == -1) {
	c[0] = geticon(i);
	c[1] = 0;
	egg_snprintf(idle, sizeof idle, "%lu", (now - dcc[i].timeval) / 60);
	list[0] = dcc[i].nick;
	list[1] = botnetnick;
	list[2] = dcc[i].host;
	list[3] = c;
	list[4] = idle;
	list[5] = dcc[i].u.chat->away ? dcc[i].u.chat->away : "";
	if (chan == -1) {
	  egg_snprintf(work, sizeof work, "%d", dcc[i].u.chat->channel);
	  list[6] = work;
	}
	p = Tcl_Merge((chan == -1) ? 7 : 6, list);
	Tcl_AppendElement(irp, p);
	Tcl_Free((char *) p);
      }
    }
  for (i = 0; i < parties; i++) {
    if (party[i].chan == chan || chan == -1) {
      c[0] = party[i].flag;
      c[1] = 0;
      if (party[i].timer == 0L)
	strcpy(idle, "0");
      else
	egg_snprintf(idle, sizeof idle, "%lu", (now - party[i].timer) / 60);
      list[0] = party[i].nick;
      list[1] = party[i].bot;
      list[2] = party[i].from ? party[i].from : "";
      list[3] = c;
      list[4] = idle;
      list[5] = party[i].status & PLSTAT_AWAY ? party[i].away : "";
      if (chan == -1) {
	egg_snprintf(work, sizeof work, "%d", party[i].chan);
	list[6] = work;
      }
      p = Tcl_Merge((chan == -1) ? 7 : 6, list);
      Tcl_AppendElement(irp, p);
      Tcl_Free((char *) p);
    }
  }
  return TCL_OK;
}

static int tcl_dccused STDVAR
{
  char s[20];

  BADARGS(1, 1, "");
  egg_snprintf(s, sizeof s, "%d", dcc_total);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

static int tcl_getdccidle STDVAR
{
  int  x, idx;
  char s[21];

  BADARGS(2, 2, " idx");
  idx = findidx(atoi(argv[1]));
  if (idx < 0) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  x = (now - dcc[idx].timeval);
  egg_snprintf(s, sizeof s, "%d", x);
  Tcl_AppendElement(irp, s);
  return TCL_OK;
}

static int tcl_getdccaway STDVAR
{
  int idx;

  BADARGS(2, 2, " idx");
  idx = findidx(atol(argv[1]));
  if (idx < 0 || dcc[idx].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (dcc[idx].u.chat->away == NULL)
    return TCL_OK;
  Tcl_AppendResult(irp, dcc[idx].u.chat->away, NULL);
  return TCL_OK;
}

static int tcl_setdccaway STDVAR
{
  int idx;

  BADARGS(3, 3, " idx message");
  idx = findidx(atol(argv[1]));
  if (idx < 0 || dcc[idx].type != &DCC_CHAT) {
    Tcl_AppendResult(irp, "invalid idx", NULL);
    return TCL_ERROR;
  }
  if (!argv[2][0]) {
    /* un-away */
    if (dcc[idx].u.chat->away != NULL)
      not_away(idx);
    return TCL_OK;
  }
  /* away */
  set_away(idx, argv[2]);
  return TCL_OK;
}

static int tcl_link STDVAR
{
  int x, i;
  char bot[HANDLEN + 1], bot2[HANDLEN + 1];

  BADARGS(2, 3, " ?via-bot? bot");
  strncpyz(bot, argv[1], sizeof bot);
  if (argc == 3) {
    x = 1;
    strncpyz(bot2, argv[2], sizeof bot2);
    i = nextbot(bot);
    if (i < 0)
      x = 0;
    else
      botnet_send_link(i, botnetnick, bot, bot2);
  } else
     x = botlink("", -2, bot);
  egg_snprintf(bot, sizeof bot, "%d", x);
  Tcl_AppendResult(irp, bot, NULL);
  return TCL_OK;
}

static int tcl_unlink STDVAR
{
  int i, x;
  char bot[HANDLEN + 1];

  BADARGS(2, 3, " bot ?comment?");
  strncpyz(bot, argv[1], sizeof bot);
  i = nextbot(bot);
  if (i < 0)
     x = 0;
  else {
    x = 1;
    if (!egg_strcasecmp(bot, dcc[i].nick))
      x = botunlink(-2, bot, argv[2]);
    else
      botnet_send_unlink(i, botnetnick, lastbot(bot), bot, argv[2]);
  }
  egg_snprintf(bot, sizeof bot, "%d", x);
  Tcl_AppendResult(irp, bot, NULL);
  return TCL_OK;
}

static int tcl_connect STDVAR
{
  int i, z, sock;
  char s[81];

  BADARGS(3, 3, " hostname port");
  if (dcc_total == max_dcc) {
    Tcl_AppendResult(irp, "out of dcc table space", NULL);
    return TCL_ERROR;
  }
  sock = getsock(0,getprotocol(argv[1]));
  if (sock < 0) {
    Tcl_AppendResult(irp, MISC_NOFREESOCK, NULL);
    return TCL_ERROR;
  }
  z = open_telnet_raw(sock, argv[1], atoi(argv[2]));
  if (z < 0) {
    killsock(sock);
    if (z == (-2))
      strncpyz(s, "DNS lookup failed", sizeof s);
    else
      neterror(s);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_ERROR;
  }
  /* Well well well... it worked! */
  i = new_dcc(&DCC_SOCKET, 0);
  dcc[i].sock = sock;
  dcc[i].port = atoi(argv[2]);
  strcpy(dcc[i].nick, "*");
  strncpyz(dcc[i].host, argv[1], UHOSTMAX);
  egg_snprintf(s, sizeof s, "%d", sock);
  Tcl_AppendResult(irp, s, NULL);
  return TCL_OK;
}

/* Create a new listening port (or destroy one)
 *
 * listen <port> bots/all/users [mask]
 * listen <port> script <proc> [flag]
 * listen <port> off
 */
static int tcl_listen STDVAR
{
  int i, j, idx = (-1), port, realport;
  char s[11];
  struct portmap *pmap = NULL, *pold = NULL;

  BADARGS(3, 5, " port type ?mask?/?proc ?flag??");
  port = realport = atoi(argv[1]);
  for (pmap = root; pmap; pold = pmap, pmap = pmap->next)
    if (pmap->realport == port) {
      port = pmap->mappedto;
      break;
    }
  for (i = 0; i < dcc_total; i++)
    if ((dcc[i].type == &DCC_TELNET) && (dcc[i].port == port))
      idx = i;
  if (!egg_strcasecmp(argv[2], "off")) {
    if (pmap) {
      if (pold)
	pold->next = pmap->next;
      else
	root = pmap->next;
      nfree(pmap);
    }
    /* Remove */
    if (idx < 0) {
      Tcl_AppendResult(irp, "no such listen port is open", NULL);
      return TCL_ERROR;
    }
    killsock(dcc[idx].sock);
    lostdcc(idx);
    return TCL_OK;
  }
  if (idx < 0) {
    /* Make new one */
    if (dcc_total >= max_dcc) {
      Tcl_AppendResult(irp, "no more DCC slots available", NULL);
      return TCL_ERROR;
    }
    /* Try to grab port */
    j = port + 20;
    i = (-1);
    while (port < j && i < 0) {
      i = open_listen(&port);
      if (i == -1)
	port++;
      else if (i == -2)
        break;
    }
    if (i == -1) {
      Tcl_AppendResult(irp, "Couldn't grab nearby port", NULL);
      return TCL_ERROR;
    } else if (i == -2) {
      Tcl_AppendResult(irp, "Couldn't assign the requested IP. Please make sure 'my-ip' is set properly.", NULL);
      return TCL_ERROR;
    }
    idx = new_dcc(&DCC_TELNET, 0);
    dcc[idx].addr = iptolong(getmyip(0));
    dcc[idx].port = port;
    dcc[idx].sock = i;
    dcc[idx].timeval = now;
  }
  /* script? */
  if (!strcmp(argv[2], "script")) {
    strcpy(dcc[idx].nick, "(script)");
    if (argc < 4) {
      Tcl_AppendResult(irp, "must give proc name for script listen", NULL);
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return TCL_ERROR;
    }
    if (argc == 5) {
      if (strcmp(argv[4], "pub")) {
	Tcl_AppendResult(irp, "unknown flag: ", argv[4], ". allowed flags: pub",
		         NULL);
	killsock(dcc[idx].sock);
	lostdcc(idx);
	return TCL_ERROR;
      }
      dcc[idx].status = LSTN_PUBLIC;
    }
    strncpyz(dcc[idx].host, argv[3], UHOSTMAX);
    egg_snprintf(s, sizeof s, "%d", port);
    Tcl_AppendResult(irp, s, NULL);
    return TCL_OK;
  }
  /* bots/users/all */
  if (!strcmp(argv[2], "bots"))
    strcpy(dcc[idx].nick, "(bots)");
  else if (!strcmp(argv[2], "users"))
    strcpy(dcc[idx].nick, "(users)");
  else if (!strcmp(argv[2], "all"))
    strcpy(dcc[idx].nick, "(telnet)");
  if (!dcc[idx].nick[0]) {
    Tcl_AppendResult(irp, "illegal listen type: must be one of ",
		     "bots, users, all, off, script", NULL);
    killsock(dcc[idx].sock);
    dcc_total--;
    return TCL_ERROR;
  }
  if (argc == 4) {
    strncpyz(dcc[idx].host, argv[3], UHOSTMAX);
  } else
    strcpy(dcc[idx].host, "*");
  egg_snprintf(s, sizeof s, "%d", port);
  Tcl_AppendResult(irp, s, NULL);
  if (!pmap) {
    pmap = nmalloc(sizeof(struct portmap));
    pmap->next = root;
    root = pmap;
  }
  pmap->realport = realport;
  pmap->mappedto = port;
  putlog(LOG_MISC, "*", "Listening at telnet port %d (%s)", port, argv[2]);
  return TCL_OK;
}

static int tcl_boot STDVAR
{
  char who[NOTENAMELEN + 1];
  int i, ok = 0;

  BADARGS(2, 3, " user@bot ?reason?");
  strncpyz(who, argv[1], sizeof who);

  if (strchr(who, '@') != NULL) {
    char whonick[HANDLEN + 1];

    splitc(whonick, who, '@');
    whonick[HANDLEN] = 0;
    if (!egg_strcasecmp(who, botnetnick))
       strncpyz(who, whonick, sizeof who);
    else if (remote_boots > 0) {
      i = nextbot(who);
      if (i < 0)
	return TCL_OK;
      botnet_send_reject(i, botnetnick, NULL, whonick, who, argv[2] ? argv[2] : "");
    } else {
      return TCL_OK;
    }
  }
  for (i = 0; i < dcc_total; i++)
    if (!ok && (dcc[i].type->flags & DCT_CANBOOT) &&
        !egg_strcasecmp(dcc[i].nick, who)) {
      do_boot(i, botnetnick, argv[2] ? argv[2] : "");
      ok = 1;
    }
  return TCL_OK;
}

static int tcl_rehash STDVAR
{
  BADARGS(1, 1, " ");
#ifdef HUB
  write_userfile(-1);
#endif
  putlog(LOG_MISC, "*", USERF_REHASHING);
  do_restart = -2;
  return TCL_OK;
}

static int tcl_restart STDVAR
{
  BADARGS(1, 1, " ");
  if (!backgrd) {
    Tcl_AppendResult(interp, "You can't restart a -n bot", NULL);
    return TCL_ERROR;
  }
#ifdef HUB
  write_userfile(-1);
#endif
  putlog(LOG_MISC, "*", MISC_RESTARTING);
  wipe_timers(interp, &utimer);
  wipe_timers(interp, &timer);
  do_restart = -1;
  return TCL_OK;
}

static int tcl_traffic STDVAR
{
  char buf[1024];
  unsigned long out_total_today, out_total;
  unsigned long in_total_today, in_total;

  /* IRC traffic */
  sprintf(buf, "irc %ld %ld %ld %ld", itraffic_irc_today, itraffic_irc +
	  itraffic_irc_today, otraffic_irc_today, otraffic_irc + otraffic_irc_today);
  Tcl_AppendElement(irp, buf);  

  /* Botnet traffic */
  sprintf(buf, "botnet %ld %ld %ld %ld", itraffic_bn_today, itraffic_bn +
          itraffic_bn_today, otraffic_bn_today, otraffic_bn + otraffic_bn_today);
  Tcl_AppendElement(irp, buf);

  /* Partyline */
  sprintf(buf, "partyline %ld %ld %ld %ld", itraffic_dcc_today, itraffic_dcc +  
          itraffic_dcc_today, otraffic_dcc_today, otraffic_dcc + otraffic_dcc_today);    
  Tcl_AppendElement(irp, buf);

  /* Transfer */
  sprintf(buf, "transfer %ld %ld %ld %ld", itraffic_trans_today, itraffic_trans +  
          itraffic_trans_today, otraffic_trans_today, otraffic_trans + otraffic_trans_today);    
  Tcl_AppendElement(irp, buf);

  /* Misc traffic */
  sprintf(buf, "misc %ld %ld %ld %ld", itraffic_unknown_today, itraffic_unknown +  
          itraffic_unknown_today, otraffic_unknown_today, otraffic_unknown + 
	  otraffic_unknown_today);    
  Tcl_AppendElement(irp, buf);

  /* Totals */
  in_total_today = itraffic_irc_today + itraffic_bn_today + itraffic_dcc_today + 
		itraffic_trans_today + itraffic_unknown_today,
  in_total = in_total_today + itraffic_irc + itraffic_bn + itraffic_dcc + 
	      itraffic_trans + itraffic_unknown;
  out_total_today = otraffic_irc_today + otraffic_bn_today + otraffic_dcc_today +
                itraffic_trans_today + otraffic_unknown_today,
  out_total = out_total_today + otraffic_irc + otraffic_bn + otraffic_dcc +
              otraffic_trans + otraffic_unknown;	  
  sprintf(buf, "total %ld %ld %ld %ld", in_total_today, in_total, out_total_today, out_total);
  Tcl_AppendElement(irp, buf);
  return(TCL_OK);
}

tcl_cmds tcldcc_cmds[] =
{
  {"binname",		tcl_binname},
  {"putdcc",		tcl_putdcc},
  {"putdccraw",		tcl_putdccraw},
  {"putidx",		tcl_putdcc},
  {"dccsimul",		tcl_dccsimul},
  {"dccbroadcast",	tcl_dccbroadcast},
  {"hand2idx",		tcl_hand2idx},
  {"getchan",		tcl_getchan},
  {"setchan",		tcl_setchan},
  {"dccputchan",	tcl_dccputchan},
  {"console",		tcl_console},
  {"strip",		tcl_strip},
  {"echo",		tcl_echo},
  {"page",		tcl_page},
  {"control",		tcl_control},
  {"valididx",		tcl_valididx},
  {"killdcc",		tcl_killdcc},
  {"putbot",		tcl_putbot},
  {"putallbots",	tcl_putallbots},
  {"idx2hand",		tcl_idx2hand},
  {"bots",		tcl_bots},
  {"botlist",		tcl_botlist},
  {"dcclist",		tcl_dcclist},
  {"whom",		tcl_whom},
  {"dccused",		tcl_dccused},
  {"getdccidle",	tcl_getdccidle},
  {"getdccaway",	tcl_getdccaway},
  {"setdccaway",	tcl_setdccaway},
  {"islinked",		tcl_islinked},
  {"hcheck",		tcl_hcheck},
  {"randstring",        tcl_randstring},
  {"source",		tcl_esource},
  {"config_end",        tcl_configend},
  {"link",		tcl_link},
  {"unlink",		tcl_unlink},
  {"connect",		tcl_connect},
  {"listen",		tcl_listen},
  {"boot",		tcl_boot},
  {"rehash",		tcl_rehash},
  {"restart",		tcl_restart},
  {"traffic",		tcl_traffic},
  {NULL,		NULL}
};
