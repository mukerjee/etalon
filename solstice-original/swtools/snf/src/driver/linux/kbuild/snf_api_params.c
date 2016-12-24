/*************************************************************************
 * The contents of this file are subject to the MYRICOM SNIFFER10G
 * LICENSE (the "License"); User may not use this file except in
 * compliance with the License.  The full text of the License can found
 * in LICENSE.TXT
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and
 * limitations under the License.
 *
 * Copyright 2009 by Myricom, Inc.  All rights reserved.
 ***********************************************************************/
#include "snf_libtypes.h"
#include "snf.h"

#ifndef MAL_KERNEL
#define U_GETENV(env) getenv(env)
#else
#define U_GETENV(env) NULL
#endif

static const char *uparam_strings[_SNF_PARAM_LAST];

#define U_INIT(u,param,type,uval,val) do {                          \
        uparam_strings[SNF_PARAM_ ## param] = _S_SNF(SNF_ ## param);    \
        (u)[SNF_PARAM_ ## param].u_set = S_DEFAULT;                     \
        (u)[SNF_PARAM_ ## param].u_type = type;                         \
        (u)[SNF_PARAM_ ## param].u_key = SNF_PARAM_ ## param;           \
        (u)[SNF_PARAM_ ## param].u_envval = U_GETENV(_S_SNF(SNF_ ## param)); \
        (u)[SNF_PARAM_ ## param].u.u_val.uval = val; } while (0);

enum uparam_setting { S_UNSET = 0, S_USER, S_ENV, S_DEFAULT };
enum uparam_op { OP_OPEN, OP_GET, OP_SET };
enum uparam_type { T_U64, T_U32, T_STR, T_PTR };

/* A single parameter stores values for what is obtained from
 * the user (uval), the default (defval) and the environment (envval).
 */
struct uparam_key {
 enum uparam_setting   u_set;
 enum uparam_type      u_type;
 enum snf_param_key    u_key;
 const char           *u_envval;

 union uparam_val {
   union snf_param_val     u_val;
   uint32_t                u_val_u32;
   uint64_t                u_val_u64;
   const char             *u_val_str;
 } u;
};

static int snf__param_op(enum uparam_op op, 
                struct snf__params *p, myri_snf_rx_params_t *drv_p,
                struct uparam_key *u);
static int snf__snptype(char *buf, size_t len, enum uparam_type type, 
                         const union uparam_val *val);
#ifndef MAL_KERNEL
static int snf__strtoul(const char *s, uint32_t *vp);
static int snf__strtoull(const char *s, uint64_t *vp);
static int snf__debugfile_reopen(struct snf__params *p, const char *filename);
#else
#define snf__strtoul(s,vp)  0
#define snf__strtoull(s,vp)  0
#endif

/*
 * Initialize library defaults and boardnum, but also query environment.
 */

#define SNF__1MB  (1024*1024)
#define SNF__2MB  (2*1024*1024)
#define SNF_ALIGNUP_2MB(x)  SNF_ALIGNUP(x, SNF__2MB)
#define SNF__DESCRING_SZ_FROM_DATARING(dataring)                \
            MIN((128ULL<<20), SNF_ALIGNUP_2MB((dataring) / 32))

int 
snf__api_params(struct snf__params *p, myri_snf_rx_params_t *drv_p,
                const struct snf_param_keyval *user_p, int num_p, 
                int *bad_param_idx)
{
  int i, j, rc = 0;
  uint64_t data_ring_size = SNF_DATARING_SZ_DEFAULT;
  struct uparam_key u[_SNF_PARAM_LAST];

  for (i = 0; i < _SNF_PARAM_LAST; i++)
    u[i].u_set = S_UNSET;

  /* 1. Fill in defaults */
  U_INIT(u, BOARDNUM, T_U32, boardnum, 0);
  U_INIT(u, NUM_RINGS, T_U32, num_rings, 1);
  U_INIT(u, RSS_FLAGS, T_U32, rss_flags, SNF_RSS_FLAGS_DEFAULT);
  U_INIT(u, RSS_FUNC_PTR, T_PTR, rss_hash_fn, NULL);
  U_INIT(u, RSS_FUNC_CONTEXT, T_PTR, rss_context, NULL);
  U_INIT(u, DATARING_SIZE, T_U64, data_ring_size, data_ring_size);
  U_INIT(u, FLAGS, T_U32, open_flags, 0);
  U_INIT(u, DEBUG_MASK, T_U32, debug_mask, 0x1);
  U_INIT(u, DEBUG_FILENAME, T_STR, debug_filename, "stderr");
  if (drv_p)
    drv_p->open_flags = 0;
#ifndef MAL_KERNEL
  p->debug_fp = stderr;
#endif

  /* 2. Process all parameters, taking over's overrides if they are set */
  for (i = 0; i < _SNF_PARAM_LAST; i++) {
    if (u[i].u_set == S_UNSET)
      continue;

    /* Find if user passed us an override to the default */
    for (j = 0; j < num_p; j++) {
      if (user_p[j].key == i) {
        /* Take user's value */
        u[i].u_set = S_USER;
        u[i].u.u_val = user_p[j].val;
        break;
      }
    }

    if ((rc = snf__param_op(OP_OPEN, p, drv_p, &u[i]))) {
      if (u[i].u_set == S_USER && (rc == EINVAL) && bad_param_idx)
        *bad_param_idx = j;
      goto out;
    }
  }

  /* 3. Print out parameters if debug is on.  */
  if (p->debug_mask & SNF_DEBUGM_PARAM) {
    char val[64];
    const char *set;

    for (i = 0; i < _SNF_PARAM_LAST; i++) {
      if (u[i].u_set == S_UNSET)
        continue;

      if (snf__snptype(val, sizeof val, u[i].u_type, &u[i].u))
        continue;

      if (u[i].u_set == S_USER)
        set = "userset";
      else if (u[i].u_set == S_ENV)
        set = "environ";
      else if (u[i].u_set == S_DEFAULT)
        set = "default";
      else
        set = "unknown";

      SNF_DPRINTF(p, PARAM, "(%7s) %24s = %-40s\n",
          set, uparam_strings[i], val);
    }
  }

  /* 4. Print extra debug_mask info when anything but WARN is set.  */
  if (p->debug_mask & ~SNF_DEBUGM_WARN) {
    const char *debug_modes[] = SNF_DEBUGM_LABELS;
    char buf[256], bufmode[64];
    uint32_t mask = p->debug_mask;
    int i;
    mal_snprintf(buf, sizeof buf, "SNF_DEBUG_MASK=0x%x for modes ", mask);
    for (i = 0; i < sizeof(debug_modes)/sizeof(debug_modes[0]); i++) {
      mask &= ~(1u<<i);
      if (debug_modes[i]) {
        mal_snprintf(bufmode, sizeof bufmode, "%s=0x%x%c ", debug_modes[i],
                 (1u<<i), mask ? ',' : ' ');
        strncat(buf, bufmode, sizeof buf - 1);
      }
    }
    SNF_DPRINTF(p, PARAM, "%s\n", buf);
  }

out:
  return rc;
}

static
int
snf__param_op(enum uparam_op op, 
             struct snf__params *p,
             myri_snf_rx_params_t *drv_p,
             struct uparam_key *u)
{
  /* Look at env var only if default value is still in use (i.e. not a
   * user-overridden value */
  const char *penv = (u->u_set == S_DEFAULT && u->u_envval && *(u->u_envval)) 
                      ? u->u_envval : NULL;
  int rc = 0;

  switch (u->u_key) {
    case SNF_PARAM_BOARDNUM:
      if (op == OP_SET) {
        rc = EPERM;
      }
      else if (op == OP_OPEN) {
        uint32_t bnum = u->u.u_val.boardnum;
        if (penv && (rc = snf__strtoul(penv, &bnum)))
          break; /* can't parse env */
        p->boardnum = bnum;
      }
      if (!rc)
        u->u.u_val.boardnum = p->boardnum;
      break;

    case SNF_PARAM_NUM_RINGS:
      if (!drv_p)
        break;
      if (op == OP_SET) {
        rc = EPERM;
      }
      else if (op == OP_OPEN) {
        uint32_t nr = u->u.u_val.num_rings;
        if (penv && (rc = snf__strtoul(penv, &nr)))
          break; /* can't parse env */
        if (nr > MYRI_SNF_MAX_RINGS)
          rc = EINVAL;
        else
          drv_p->num_rings = nr;
      }
      if (!rc)
        u->u.u_val.num_rings = drv_p->num_rings;
      break;

    case SNF_PARAM_RSS_FLAGS:
      if (!drv_p)
        break;
      if (op == OP_SET) {
        rc = EPERM;
      }
      else if (op == OP_OPEN) {
        uint32_t flags = u->u.u_val.rss_flags;
        if (penv && (rc = snf__strtoul(penv, &flags)))
          break; /* can't parse env */
        drv_p->rss_flags = flags;
      }
      if (!rc)
        u->u.u_val.rss_flags = drv_p->rss_flags;
      break;

    case SNF_PARAM_RSS_FUNC_PTR:
      if (op == OP_SET) {
        rc = EPERM;
      }
      else if (op == OP_OPEN) {
        if (u->u_set == S_USER && u->u.u_val.rss_hash_fn == NULL)
          rc = EINVAL;
        else
          p->rss_hash_fn = u->u.u_val.rss_hash_fn;
      }
      break;

    case SNF_PARAM_RSS_FUNC_CONTEXT:
      if (op == OP_SET) {
        rc = EPERM;
      }
      else if (op == OP_OPEN)
        p->rss_context = u->u.u_val.rss_context;
      break;

    case SNF_PARAM_DATARING_SIZE:
      if (!drv_p)
        break;
      if (op == OP_SET) {
        rc = EPERM;
      }
      else if (op == OP_OPEN) {
        uint64_t data_ring_size = u->u.u_val.data_ring_size;
        if (penv && (rc = snf__strtoull(penv, &data_ring_size)))
          break;
        if (data_ring_size < SNF__1MB)
          data_ring_size *= SNF__1MB;
        data_ring_size = SNF_ALIGNUP_2MB(data_ring_size);
        drv_p->rx_data_ring_size = data_ring_size;
        drv_p->rx_desc_ring_size = MIN((128ULL<<20), 
                                     SNF_ALIGNUP_2MB(data_ring_size / 32));
      }
      if (!rc)
        u->u.u_val.data_ring_size = drv_p->rx_data_ring_size;
      break;

    case SNF_PARAM_FLAGS:
      if (!drv_p)
        break;
      if (op == OP_SET) {
        rc = EPERM;
      }
      else if (op == OP_OPEN) {
        uint32_t open_flags = u->u.u_val.open_flags;
        if (penv && (rc = snf__strtoul(penv, &open_flags)))
          break;
        drv_p->open_flags = open_flags;
      }
      if (!rc)
        u->u.u_val.open_flags = drv_p->open_flags;
      break;

    case SNF_PARAM_DEBUG_FILENAME:
#ifndef MAL_KERNEL
      if (op == OP_SET || op == OP_OPEN) {
        const char *str = penv ? penv : u->u.u_val.debug_filename;
        if (str == NULL) {
          rc = EINVAL;
          break;
        }
        else {
          rc = snf__debugfile_reopen(p, str);
        }
      }
      if (!rc)
        u->u.u_val.debug_filename = p->debug_filename;
#endif
      break;

    case SNF_PARAM_DEBUG_MASK:
      if (op == OP_SET || op == OP_OPEN) {
        uint32_t dmask = u->u.u_val.debug_mask;
        if (penv && (rc = snf__strtoul(penv, &dmask)))
          break;
        /* always set 0x1 */
        p->debug_mask = dmask | 0x1;
      }
      if (!rc)
        u->u.u_val.debug_mask = p->debug_mask;
      break;

    case SNF_PARAM_TIMESYNC_INTERVAL:
      break;

      default:
        rc = ENOENT;
        break;
  }
  if (!rc && penv)
    u->u_set = S_ENV;

  return rc;

}

static 
int 
snf__snptype(char *buf, size_t len, enum uparam_type type, 
             const union uparam_val *u)
{
  size_t nob;
  buf[0] = '\0';

  switch (type) {
    case T_U64:
      nob = mal_snprintf(buf, len, "%llu", (unsigned long long) u->u_val_u64);
      if (u->u_val_u64)
        nob += mal_snprintf(buf + nob, len - nob, " (%#llx)", (long long) u->u_val_u64);
      if (u->u_val_u64 > 1024*1024 && nob*2 < len)
        mal_snprintf(buf + nob, len - nob, " (%.1f MiB)", (double) u->u_val_u64 / (1024*1024));
      break;
    case T_U32:
      if (u->u_val_u32 != 0)
        mal_snprintf(buf, len, "%u (%#x)", u->u_val_u32, u->u_val_u32);
      else
        mal_snprintf(buf, len, "%u", u->u_val_u32);
      break;
    case T_STR:
      mal_snprintf(buf, len, "%s", u->u_val_str);
      break;
    default:
      return -1;
      break;
  }
  buf[len-1] = '\0';
  return 0;
}

#ifndef MAL_KERNEL
static
int
snf__strtoul(const char *s, uint32_t *vp)
{
  uint32_t v;
  errno = 0;
  v = (uint32_t) strtoul(s, NULL, 0);
  return errno ? (errno=0, ERANGE) : (*vp = v, 0);
}

static
int
snf__strtoull(const char *s, uint64_t *vp)
{
  uint64_t v;
  errno = 0;
  v = (uint64_t) strtoull(s, NULL, 0);
  return errno ? (errno=0, ERANGE) : (*vp = v, 0);
}

static
int
snf__debugfile_reopen(struct snf__params *p, const char *filename)
{
  FILE *newfp, *oldfp = p->debug_fp;
  const char *fmode = "w";
  const char *s = p->debug_file_buf;

  strncpy(p->debug_file_buf, filename, sizeof p->debug_file_buf);
  p->debug_file_buf[sizeof p->debug_file_buf - 1] = '\0';

  /* Look for append marker */
  if (p->debug_file_buf[0] == '+') {
    fmode = "a";
    s++;
  }

  if (oldfp != stderr && oldfp != stdout && oldfp != NULL)
    fclose(oldfp);
  if (strncmp(s, "stderr", strlen("stderr")+1) == 0)
    newfp = stderr;
  else if (strncmp(s, "stdout", strlen("stdout")+1) == 0)
    newfp = stdout;
  else {
    newfp = fopen(s, fmode);
    if (newfp == NULL) {
      fprintf(stderr, "Can't open '%s', forcing DEBUG_MASK to 0x1 (errno=%d: %s)\n",
              s, errno, strerror(errno));
      p->debug_mask = 0x1;
      return 0;
    }
  }

  p->debug_fp = newfp;
  p->debug_filename = p->debug_file_buf;

  return 0;
}
#endif
