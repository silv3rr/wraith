#define MODULE_NAME "encryption"
#define MAKING_ENCRYPTION
#include "src/mod/module.h"
#include "blowfish.h"
#include "bf_tab.h"
#undef global
static Function *global = NULL;
#define BOXES 3
#define S0(x) (bf_S[0][x.w.byte0])
#define S1(x) (bf_S[1][x.w.byte1])
#define S2(x) (bf_S[2][x.w.byte2])
#define S3(x) (bf_S[3][x.w.byte3])
#define bf_F(x) (((S0(x) + S1(x)) ^ S2(x)) + S3(x))
#define ROUND(a,b,n) (a.word ^= bf_F(b) ^ bf_P[n])
static struct box_t
{
  u_32bit_t *P;
  u_32bit_t **S;
  char key[81];
  char keybytes;
  time_t lastuse;
} box[BOXES];
static u_32bit_t *bf_P;
static u_32bit_t **bf_S;
static int
blowfish_expmem ()
{
  int i, tot = 0;
  for (i = 0; i < BOXES; i++)
    if (box[i].P != NULL)
      {
	tot += ((bf_N + 2) * sizeof (u_32bit_t));
	tot += (4 * sizeof (u_32bit_t *));
	tot += (4 * 256 * sizeof (u_32bit_t));
      }
  return tot;
}
static void
blowfish_encipher (u_32bit_t * xl, u_32bit_t * xr)
{
  union aword Xl;
  union aword Xr;
  Xl.word = *xl;
  Xr.word = *xr;
  Xl.word ^= bf_P[0];
  ROUND (Xr, Xl, 1);
  ROUND (Xl, Xr, 2);
  ROUND (Xr, Xl, 3);
  ROUND (Xl, Xr, 4);
  ROUND (Xr, Xl, 5);
  ROUND (Xl, Xr, 6);
  ROUND (Xr, Xl, 7);
  ROUND (Xl, Xr, 8);
  ROUND (Xr, Xl, 9);
  ROUND (Xl, Xr, 10);
  ROUND (Xr, Xl, 11);
  ROUND (Xl, Xr, 12);
  ROUND (Xr, Xl, 13);
  ROUND (Xl, Xr, 14);
  ROUND (Xr, Xl, 15);
  ROUND (Xl, Xr, 16);
  Xr.word ^= bf_P[17];
  *xr = Xl.word;
  *xl = Xr.word;
} static void

blowfish_decipher (u_32bit_t * xl, u_32bit_t * xr)
{
  union aword Xl;
  union aword Xr;
  Xl.word = *xl;
  Xr.word = *xr;
  Xl.word ^= bf_P[17];
  ROUND (Xr, Xl, 16);
  ROUND (Xl, Xr, 15);
  ROUND (Xr, Xl, 14);
  ROUND (Xl, Xr, 13);
  ROUND (Xr, Xl, 12);
  ROUND (Xl, Xr, 11);
  ROUND (Xr, Xl, 10);
  ROUND (Xl, Xr, 9);
  ROUND (Xr, Xl, 8);
  ROUND (Xl, Xr, 7);
  ROUND (Xr, Xl, 6);
  ROUND (Xl, Xr, 5);
  ROUND (Xr, Xl, 4);
  ROUND (Xl, Xr, 3);
  ROUND (Xr, Xl, 2);
  ROUND (Xl, Xr, 1);
  Xr.word ^= bf_P[0];
  *xl = Xr.word;
  *xr = Xl.word;
} static void
blowfish_report (int idx, int details)
{
  if (details)
    {
      int i, tot = 0, size = blowfish_expmem ();
      for (i = 0; i < BOXES; i++)
	if (box[i].P != NULL)
	  tot++;
      dprintf (idx, "    Blowfish encryption module:\n");
      dprintf (idx, "    %d of %d boxes in use: ", tot, BOXES);
      for (i = 0; i < BOXES; i++)
	if (box[i].P != NULL)
	  {
	    dprintf (idx, "(age: %d) ", now - box[i].lastuse);
	  }
      dprintf (idx, "\n");
      dprintf (idx, "  Using %d byte%s of memory\n", size,
	       (size != 1) ? "s" : "");
    }
}
static void
blowfish_init (u_8bit_t * key, int keybytes)
{
  int i, j, bx;
  time_t lowest;
  u_32bit_t data;
  u_32bit_t datal;
  u_32bit_t datar;
  union aword temp;
  if (keybytes > 80)
    keybytes = 80;
  for (i = 0; i < BOXES; i++)
    if (box[i].P != NULL)
      {
	if ((box[i].keybytes == keybytes)
	    && (!strncmp ((char *) (box[i].key), (char *) key, keybytes)))
	  {
	    box[i].lastuse = now;
	    bf_P = box[i].P;
	    bf_S = box[i].S;
	    return;
	  }
      }
  bx = (-1);
  for (i = 0; i < BOXES; i++)
    {
      if (box[i].P == NULL)
	{
	  bx = i;
	  i = BOXES + 1;
	}
    }
  if (bx < 0)
    {
      lowest = now;
      for (i = 0; i < BOXES; i++)
	if (box[i].lastuse <= lowest)
	  {
	    lowest = box[i].lastuse;
	    bx = i;
	  }
      nfree (box[bx].P);
      for (i = 0; i < 4; i++)
	nfree (box[bx].S[i]);
      nfree (box[bx].S);
    }
  box[bx].P = (u_32bit_t *) nmalloc ((bf_N + 2) * sizeof (u_32bit_t));
  box[bx].S = (u_32bit_t **) nmalloc (4 * sizeof (u_32bit_t *));
  for (i = 0; i < 4; i++)
    box[bx].S[i] = (u_32bit_t *) nmalloc (256 * sizeof (u_32bit_t));
  bf_P = box[bx].P;
  bf_S = box[bx].S;
  box[bx].keybytes = keybytes;
  strncpy (box[bx].key, key, keybytes);
  box[bx].key[keybytes] = 0;
  box[bx].lastuse = now;
  for (i = 0; i < bf_N + 2; i++)
    bf_P[i] = initbf_P[i];
  for (i = 0; i < 4; i++)
    for (j = 0; j < 256; j++)
      bf_S[i][j] = initbf_S[i][j];
  j = 0;
  if (keybytes > 0)
    {
      for (i = 0; i < bf_N + 2; ++i)
	{
	  temp.word = 0;
	  temp.w.byte0 = key[j];
	  temp.w.byte1 = key[(j + 1) % keybytes];
	  temp.w.byte2 = key[(j + 2) % keybytes];
	  temp.w.byte3 = key[(j + 3) % keybytes];
	  data = temp.word;
	  bf_P[i] = bf_P[i] ^ data;
	  j = (j + 4) % keybytes;
	}
    }
  datal = 0x00000000;
  datar = 0x00000000;
  for (i = 0; i < bf_N + 2; i += 2)
    {
      blowfish_encipher (&datal, &datar);
      bf_P[i] = datal;
      bf_P[i + 1] = datar;
    }
  for (i = 0; i < 4; ++i)
    {
      for (j = 0; j < 256; j += 2)
	{
	  blowfish_encipher (&datal, &datar);
	  bf_S[i][j] = datal;
	  bf_S[i][j + 1] = datar;
	}
    }
}

#define SALT1 0xdeadd061
#define SALT2 0x23f6b095
static char *base64 =
  "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int
base64dec (char c)
{
  int i;
  for (i = 0; i < 64; i++)
    if (base64[i] == c)
      return i;
  return 0;
}
static void
blowfish_encrypt_pass (char *text, char *new)
{
  u_32bit_t left, right;
  int n;
  char *p;
  blowfish_init ((unsigned char *) text, strlen (text));
  left = SALT1;
  right = SALT2;
  blowfish_encipher (&left, &right);
  p = new;
  *p++ = '+';
  n = 32;
  while (n > 0)
    {
      *p++ = base64[right & 0x3f];
      right = (right >> 6);
      n -= 6;
    }
  n = 32;
  while (n > 0)
    {
      *p++ = base64[left & 0x3f];
      left = (left >> 6);
      n -= 6;
    }
  *p = 0;
}
static char *
encrypt_string (char *key, char *str)
{
  u_32bit_t left, right;
  unsigned char *p;
  char *s, *dest, *d;
  int i;
  s = (char *) nmalloc (strlen (str) + 9);
  strcpy (s, str);
  if ((!key) || (!key[0]))
    return s;
  p = s;
  dest = (char *) nmalloc ((strlen (str) + 9) * 2);
  while (*p)
    p++;
  for (i = 0; i < 8; i++)
    *p++ = 0;
  blowfish_init ((unsigned char *) key, strlen (key));
  p = s;
  d = dest;
  while (*p)
    {
      left = ((*p++) << 24);
      left += ((*p++) << 16);
      left += ((*p++) << 8);
      left += (*p++);
      right = ((*p++) << 24);
      right += ((*p++) << 16);
      right += ((*p++) << 8);
      right += (*p++);
      blowfish_encipher (&left, &right);
      for (i = 0; i < 6; i++)
	{
	  *d++ = base64[right & 0x3f];
	  right = (right >> 6);
	}
      for (i = 0; i < 6; i++)
	{
	  *d++ = base64[left & 0x3f];
	  left = (left >> 6);
	}
    }
  *d = 0;
  nfree (s);
  return dest;
}
static char *
decrypt_string (char *key, char *str)
{
  u_32bit_t left, right;
  char *p, *s, *dest, *d;
  int i;
  s = (char *) nmalloc (strlen (str) + 12);
  strcpy (s, str);
  if ((!key) || (!key[0]))
    return s;
  p = s;
  dest = (char *) nmalloc (strlen (str) + 12);
  while (*p)
    p++;
  for (i = 0; i < 12; i++)
    *p++ = 0;
  blowfish_init ((unsigned char *) key, strlen (key));
  p = s;
  d = dest;
  while (*p)
    {
      right = 0L;
      left = 0L;
      for (i = 0; i < 6; i++)
	right |= (base64dec (*p++)) << (i * 6);
      for (i = 0; i < 6; i++)
	left |= (base64dec (*p++)) << (i * 6);
      blowfish_decipher (&left, &right);
      for (i = 0; i < 4; i++)
	*d++ = (left & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
      for (i = 0; i < 4; i++)
	*d++ = (right & (0xff << ((3 - i) * 8))) >> ((3 - i) * 8);
    }
  *d = 0;
  nfree (s);
  return dest;
}
static int tcl_encrypt STDVAR
{
  char *p;
    BADARGS (3, 3, " key string");
    p = encrypt_string (argv[1], argv[2]);
    Tcl_AppendResult (irp, p, NULL);
    nfree (p);
    return TCL_OK;
}
static int tcl_decrypt STDVAR
{
  char *p;
    BADARGS (3, 3, " key string");
    p = decrypt_string (argv[1], argv[2]);
    Tcl_AppendResult (irp, p, NULL);
    nfree (p);
    return TCL_OK;
}
static int tcl_encpass STDVAR
{
  BADARGS (2, 2, " string");
  if (strlen (argv[1]) > 0)
    {
      char p[16];
        blowfish_encrypt_pass (argv[1], p);
        Tcl_AppendResult (irp, p, NULL);
    }
  else
      Tcl_AppendResult (irp, "", NULL);
  return TCL_OK;
}
static tcl_cmds mytcls[] =
  { {"encrypt", tcl_encrypt}, {"decrypt", tcl_decrypt}, {"encpass",
							 tcl_encpass}, {NULL,
									NULL}
};
static char *
blowfish_close ()
{
  return "You can't unload the encryption module";
}
EXPORT_SCOPE char *blowfish_start (Function *);
static Function blowfish_table[] =
  { (Function) blowfish_start, (Function) blowfish_close,
(Function) blowfish_expmem, (Function) blowfish_report, (Function) encrypt_string,
(Function) decrypt_string, };
char *
blowfish_start (Function * global_funcs)
{
  int i;
  if (global_funcs)
    {
      global = global_funcs;
      if (!module_rename ("blowfish", MODULE_NAME))
	return "Already loaded.";
      for (i = 0; i < BOXES; i++)
	{
	  box[i].P = NULL;
	  box[i].S = NULL;
	  box[i].key[0] = 0;
	  box[i].lastuse = 0L;
	}
      module_register (MODULE_NAME, blowfish_table, 2, 1);
      add_hook (HOOK_ENCRYPT_PASS, (Function) blowfish_encrypt_pass);
      add_hook (HOOK_ENCRYPT_STRING, (Function) encrypt_string);
      add_hook (HOOK_DECRYPT_STRING, (Function) decrypt_string);
    }
  add_tcl_commands (mytcls);
  return NULL;
}
