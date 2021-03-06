/* Copyright (C) 2009-2011 Codership Oy <info@codersihp.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*! @file wsrep implementation loader */
#ifndef _MSC_VER
#include <dlfcn.h>
#else
#include <windows.h>
#define dlsym(lib, name) (void*)GetProcAddress((HMODULE)lib, name)
#define dlopen(libname, unused) LoadLibraryEx(libname, NULL, 0)
#define dlclose(lib) FreeLibrary((HMODULE)lib)
static __inline char *dlerror(void)
{
  static char win_errormsg[2048];
  if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                   0, GetLastError(), 0, win_errormsg, 2048, NULL))
    return win_errormsg;
  return "";
}
#define HAVE_DLOPEN 1
#define HAVE_DLERROR 1

#endif
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "wsrep_api.h"
#ifdef _MSC_VER
#define snprintf _snprintf
#endif
// Logging stuff for the loader
static const char* log_levels[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG"};

static void default_logger (wsrep_log_level_t lvl, const char* msg)
{
    fprintf (stderr, "wsrep loader: [%s] %s\n", log_levels[lvl], msg);
}

static wsrep_log_cb_t logger = default_logger;

/**************************************************************************
 * Library loader
 **************************************************************************/

static int verify(const wsrep_t *wh, const char *iface_ver)
{
    const size_t msg_len = 128;
    char *msg = alloca(sizeof(char)*msg_len);

#define VERIFY(_p) if (!(_p)) {                                       \
        snprintf(msg, msg_len, "wsrep_load(): verify(): %s\n", # _p); \
        logger (WSREP_LOG_ERROR, msg);                                \
        return EINVAL;                                                \
    }

    VERIFY(wh);
    VERIFY(wh->version);

    if (strcmp(wh->version, iface_ver)) {
        snprintf (msg, msg_len,
                  "provider interface version mismatch: need '%s', found '%s'",
                  iface_ver, wh->version);
        logger (WSREP_LOG_ERROR, msg);
        return EINVAL;
    }

    VERIFY(wh->init);
    VERIFY(wh->options_set);
    VERIFY(wh->options_get);
    VERIFY(wh->connect);
    VERIFY(wh->disconnect);
    VERIFY(wh->recv);
    VERIFY(wh->pre_commit);
    VERIFY(wh->post_commit);
    VERIFY(wh->post_rollback);
    VERIFY(wh->replay_trx);
    VERIFY(wh->abort_pre_commit);
    VERIFY(wh->append_query);
    VERIFY(wh->append_key);
    VERIFY(wh->free_connection);
    VERIFY(wh->to_execute_start);
    VERIFY(wh->to_execute_end);
    VERIFY(wh->sst_sent);
    VERIFY(wh->sst_received);
    VERIFY(wh->stats_get);
    VERIFY(wh->stats_free);
    VERIFY(wh->pause);
    VERIFY(wh->resume);
    VERIFY(wh->desync);
    VERIFY(wh->resync);
    VERIFY(wh->lock);
    VERIFY(wh->unlock);
    VERIFY(wh->is_locked);
    VERIFY(wh->provider_name);
    VERIFY(wh->provider_version);
    VERIFY(wh->provider_vendor);
    VERIFY(wh->free);
    return 0;
}


static wsrep_loader_fun wsrep_dlf(void *dlh, const char *sym)
{
#ifdef _MSC_VER
	return (wsrep_loader_fun)dlsym(dlh, sym);
#else

    union {
        wsrep_loader_fun dlfun;
        void *obj;
    } alias;
    alias.obj = dlsym(dlh, sym);
    return alias.dlfun;
#endif
}

extern int wsrep_dummy_loader(wsrep_t *w);

int wsrep_load(const char *spec, wsrep_t **hptr, wsrep_log_cb_t log_cb)
{
    int ret = 0;
    void *dlh = NULL;
    wsrep_loader_fun dlfun;
    const size_t msg_len = 1024;
	wsrep_t *hptr0;
    char *msg=alloca(sizeof(char)*(msg_len + 1));
    msg[msg_len] = 0;

    if (NULL != log_cb)
        logger = log_cb;

    if (!(spec && hptr))
        return EINVAL;

    snprintf (msg, msg_len,
              "wsrep_load(): loading provider library '%s'", spec);
    logger (WSREP_LOG_INFO, msg);

    if (!(*hptr = malloc(sizeof(wsrep_t)))) {
        logger (WSREP_LOG_FATAL, "wsrep_load(): out of memory");
        return ENOMEM;
    }
	hptr0=*hptr;
    if (!spec || strcmp(spec, WSREP_NONE) == 0) {
        if ((ret = wsrep_dummy_loader(hptr0)) != 0) {
            free (hptr0);
            *hptr = NULL;
        }
        return ret;
    }

    if (!(dlh = dlopen(spec, RTLD_NOW | RTLD_LOCAL))) {
        snprintf(msg, msg_len, "wsrep_load(): dlopen(): %s", dlerror());
        logger (WSREP_LOG_ERROR, msg);
        ret = EINVAL;
        goto out;
    }

    if (!(dlfun = wsrep_dlf(dlh, "wsrep_loader"))) {
        ret = EINVAL;
        goto out;
    }

    if ((ret = (*dlfun)(hptr0)) != 0) {
        snprintf(msg, msg_len, "wsrep_load(): loader failed: %s",
                 strerror(ret));
        logger (WSREP_LOG_ERROR, msg);
        goto out;
    }

    if ((ret = verify(hptr0, WSREP_INTERFACE_VERSION)) != 0) {
        snprintf (msg, msg_len,
                  "wsrep_load(): interface version mismatch: my version %s, "
                  "provider version %s", WSREP_INTERFACE_VERSION,
                  hptr0->version);
        logger (WSREP_LOG_ERROR, msg);
        goto out;
    }

    hptr0->dlh = dlh;

out:
    if (ret != 0) {
        if (dlh) dlclose(dlh);
        free(hptr0);
        *hptr = NULL;
    } else {
        snprintf (msg, msg_len,
                  "wsrep_load(): %s %s by %s loaded succesfully.",
                  hptr0->provider_name, hptr0->provider_version,
                  hptr0->provider_vendor);
        logger (WSREP_LOG_INFO, msg);
    }

    return ret;
}

void wsrep_unload(wsrep_t *hptr)
{
    if (!hptr) {
        logger (WSREP_LOG_WARN, "wsrep_unload(): null pointer.");
    } else {
        if (hptr->free)
            hptr->free(hptr);
        if (hptr->dlh)
            dlclose(hptr->dlh);
        free(hptr);
    }
}

