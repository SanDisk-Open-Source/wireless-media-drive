/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* dbus-sysdeps-util-unix.c Would be in dbus-sysdeps-unix.c, but not used in libdbus
 * 
 * Copyright (C) 2002, 2003, 2004, 2005  Red Hat, Inc.
 * Copyright (C) 2003 CodeFactory AB
 *
 * Licensed under the Academic Free License version 2.1
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "dbus-sysdeps.h"
#include "dbus-sysdeps-unix.h"
#include "dbus-internals.h"
#include "dbus-protocol.h"
#include "dbus-string.h"
#define DBUS_USERDB_INCLUDES_PRIVATE 1
#include "dbus-userdb.h"
#include "dbus-test.h"

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <grp.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/un.h>
#ifdef HAVE_LIBAUDIT
#include <sys/prctl.h>
#include <sys/capability.h>
#include <libaudit.h>
#endif /* HAVE_LIBAUDIT */

#ifdef HAVE_SYS_SYSLIMITS_H
#include <sys/syslimits.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

/**
 * @addtogroup DBusInternalsUtils
 * @{
 */


/**
 * Does the chdir, fork, setsid, etc. to become a daemon process.
 *
 * @param pidfile #NULL, or pidfile to create
 * @param print_pid_pipe pipe to print daemon's pid to, or -1 for none
 * @param error return location for errors
 * @returns #FALSE on failure
 */
dbus_bool_t
_dbus_become_daemon (const DBusString *pidfile,
                     DBusPipe         *print_pid_pipe,
                     DBusError        *error)
{
  const char *s;
  pid_t child_pid;
  int dev_null_fd;

  _dbus_verbose ("Becoming a daemon...\n");

  _dbus_verbose ("chdir to /\n");
  if (chdir ("/") < 0)
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
                      "Could not chdir() to root directory");
      return FALSE;
    }

  _dbus_verbose ("forking...\n");
  switch ((child_pid = fork ()))
    {
    case -1:
      _dbus_verbose ("fork failed\n");
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to fork daemon: %s", _dbus_strerror (errno));
      return FALSE;
      break;

    case 0:
      _dbus_verbose ("in child, closing std file descriptors\n");

      /* silently ignore failures here, if someone
       * doesn't have /dev/null we may as well try
       * to continue anyhow
       */
      
      dev_null_fd = open ("/dev/null", O_RDWR);
      if (dev_null_fd >= 0)
        {
          dup2 (dev_null_fd, 0);
          dup2 (dev_null_fd, 1);
          
          s = _dbus_getenv ("DBUS_DEBUG_OUTPUT");
          if (s == NULL || *s == '\0')
            dup2 (dev_null_fd, 2);
          else
            _dbus_verbose ("keeping stderr open due to DBUS_DEBUG_OUTPUT\n");
        }

      /* Get a predictable umask */
      _dbus_verbose ("setting umask\n");
      umask (022);

      _dbus_verbose ("calling setsid()\n");
      if (setsid () == -1)
        _dbus_assert_not_reached ("setsid() failed");
      
      break;

    default:
      if (!_dbus_write_pid_to_file_and_pipe (pidfile, print_pid_pipe,
                                             child_pid, error))
        {
          _dbus_verbose ("pid file or pipe write failed: %s\n",
                         error->message);
          kill (child_pid, SIGTERM);
          return FALSE;
        }

      _dbus_verbose ("parent exiting\n");
      _exit (0);
      break;
    }
  
  return TRUE;
}


/**
 * Creates a file containing the process ID.
 *
 * @param filename the filename to write to
 * @param pid our process ID
 * @param error return location for errors
 * @returns #FALSE on failure
 */
static dbus_bool_t
_dbus_write_pid_file (const DBusString *filename,
                      unsigned long     pid,
		      DBusError        *error)
{
  const char *cfilename;
  int fd;
  FILE *f;

  cfilename = _dbus_string_get_const_data (filename);
  
  fd = open (cfilename, O_WRONLY|O_CREAT|O_EXCL|O_BINARY, 0644);
  
  if (fd < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to open \"%s\": %s", cfilename,
                      _dbus_strerror (errno));
      return FALSE;
    }

  if ((f = fdopen (fd, "w")) == NULL)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to fdopen fd %d: %s", fd, _dbus_strerror (errno));
      _dbus_close (fd, NULL);
      return FALSE;
    }
  
  if (fprintf (f, "%lu\n", pid) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to write to \"%s\": %s", cfilename,
                      _dbus_strerror (errno));
      
      fclose (f);
      return FALSE;
    }

  if (fclose (f) == EOF)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to close \"%s\": %s", cfilename,
                      _dbus_strerror (errno));
      return FALSE;
    }
  
  return TRUE;
}

/**
 * Writes the given pid_to_write to a pidfile (if non-NULL) and/or to a
 * pipe (if non-NULL). Does nothing if pidfile and print_pid_pipe are both
 * NULL.
 *
 * @param pidfile the file to write to or #NULL
 * @param print_pid_pipe the pipe to write to or #NULL
 * @param pid_to_write the pid to write out
 * @param error error on failure
 * @returns FALSE if error is set
 */
dbus_bool_t
_dbus_write_pid_to_file_and_pipe (const DBusString *pidfile,
                                  DBusPipe         *print_pid_pipe,
                                  dbus_pid_t        pid_to_write,
                                  DBusError        *error)
{
  if (pidfile)
    {
      _dbus_verbose ("writing pid file %s\n", _dbus_string_get_const_data (pidfile));
      if (!_dbus_write_pid_file (pidfile,
                                 pid_to_write,
                                 error))
        {
          _dbus_verbose ("pid file write failed\n");
          _DBUS_ASSERT_ERROR_IS_SET(error);
          return FALSE;
        }
    }
  else
    {
      _dbus_verbose ("No pid file requested\n");
    }

  if (print_pid_pipe != NULL && _dbus_pipe_is_valid (print_pid_pipe))
    {
      DBusString pid;
      int bytes;

      _dbus_verbose ("writing our pid to pipe %d\n", print_pid_pipe->fd_or_handle);
      
      if (!_dbus_string_init (&pid))
        {
          _DBUS_SET_OOM (error);
          return FALSE;
        }
	  
      if (!_dbus_string_append_int (&pid, pid_to_write) ||
          !_dbus_string_append (&pid, "\n"))
        {
          _dbus_string_free (&pid);
          _DBUS_SET_OOM (error);
          return FALSE;
        }
	  
      bytes = _dbus_string_get_length (&pid);
      if (_dbus_pipe_write (print_pid_pipe, &pid, 0, bytes, error) != bytes)
        {
          /* _dbus_pipe_write sets error only on failure, not short write */
          if (error != NULL && !dbus_error_is_set(error))
            {
              dbus_set_error (error, DBUS_ERROR_FAILED,
                              "Printing message bus PID: did not write enough bytes\n");
            }
          _dbus_string_free (&pid);
          return FALSE;
        }
	  
      _dbus_string_free (&pid);
    }
  else
    {
      _dbus_verbose ("No pid pipe to write to\n");
    }

  return TRUE;
}

/**
 * Verify that after the fork we can successfully change to this user.
 *
 * @param user the username given in the daemon configuration
 * @returns #TRUE if username is valid
 */
dbus_bool_t
_dbus_verify_daemon_user (const char *user)
{
  DBusString u;

  _dbus_string_init_const (&u, user);

  return _dbus_get_user_id_and_primary_group (&u, NULL, NULL);
}

/**
 * Changes the user and group the bus is running as.
 *
 * @param user the user to become
 * @param error return location for errors
 * @returns #FALSE on failure
 */
dbus_bool_t
_dbus_change_to_daemon_user  (const char    *user,
                              DBusError     *error)
{
  dbus_uid_t uid;
  dbus_gid_t gid;
  DBusString u;
#ifdef HAVE_LIBAUDIT
  dbus_bool_t we_were_root;
  cap_t new_caps;
#endif
  
  _dbus_string_init_const (&u, user);
  
  if (!_dbus_get_user_id_and_primary_group (&u, &uid, &gid))
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
                      "User '%s' does not appear to exist?",
                      user);
      return FALSE;
    }
  
#ifdef HAVE_LIBAUDIT
  we_were_root = _dbus_geteuid () == 0;
  new_caps = NULL;
  /* have a tmp set of caps that we use to transition to the usr/grp dbus should
   * run as ... doesn't really help. But keeps people happy.
   */
    
  if (we_were_root)
    {
      cap_value_t new_cap_list[] = { CAP_AUDIT_WRITE };
      cap_value_t tmp_cap_list[] = { CAP_AUDIT_WRITE, CAP_SETUID, CAP_SETGID };
      cap_t tmp_caps = cap_init();
        
      if (!tmp_caps || !(new_caps = cap_init ()))
        {
          dbus_set_error (error, DBUS_ERROR_FAILED,
                          "Failed to initialize drop of capabilities: %s\n",
                          _dbus_strerror (errno));

          if (tmp_caps)
            cap_free (tmp_caps);

          return FALSE;
        }

      /* assume these work... */
      cap_set_flag (new_caps, CAP_PERMITTED, 1, new_cap_list, CAP_SET);
      cap_set_flag (new_caps, CAP_EFFECTIVE, 1, new_cap_list, CAP_SET);
      cap_set_flag (tmp_caps, CAP_PERMITTED, 3, tmp_cap_list, CAP_SET);
      cap_set_flag (tmp_caps, CAP_EFFECTIVE, 3, tmp_cap_list, CAP_SET);
      
      if (prctl (PR_SET_KEEPCAPS, 1, 0, 0, 0) == -1)
        {
          dbus_set_error (error, _dbus_error_from_errno (errno),
                          "Failed to set keep-capabilities: %s\n",
                          _dbus_strerror (errno));
          cap_free (tmp_caps);
          goto fail;
        }
        
      if (cap_set_proc (tmp_caps) == -1)
        {
          dbus_set_error (error, DBUS_ERROR_FAILED,
                          "Failed to drop capabilities: %s\n",
                          _dbus_strerror (errno));
          cap_free (tmp_caps);
          goto fail;
        }
      cap_free (tmp_caps);
    }
#endif /* HAVE_LIBAUDIT */
  
  /* setgroups() only works if we are a privileged process,
   * so we don't return error on failure; the only possible
   * failure is that we don't have perms to do it.
   *
   * not sure this is right, maybe if setuid()
   * is going to work then setgroups() should also work.
   */
  if (setgroups (0, NULL) < 0)
    _dbus_warn ("Failed to drop supplementary groups: %s\n",
                _dbus_strerror (errno));
  
  /* Set GID first, or the setuid may remove our permission
   * to change the GID
   */
  if (setgid (gid) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to set GID to %lu: %s", gid,
                      _dbus_strerror (errno));
      goto fail;
    }
  
  if (setuid (uid) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to set UID to %lu: %s", uid,
                      _dbus_strerror (errno));
      goto fail;
    }
  
#ifdef HAVE_LIBAUDIT
  if (we_were_root)
    {
      if (cap_set_proc (new_caps))
        {
          dbus_set_error (error, DBUS_ERROR_FAILED,
                          "Failed to drop capabilities: %s\n",
                          _dbus_strerror (errno));
          goto fail;
        }
      cap_free (new_caps);

      /* should always work, if it did above */      
      if (prctl (PR_SET_KEEPCAPS, 0, 0, 0, 0) == -1)
        {
          dbus_set_error (error, _dbus_error_from_errno (errno),
                          "Failed to unset keep-capabilities: %s\n",
                          _dbus_strerror (errno));
          return FALSE;
        }
    }
#endif

 return TRUE;

 fail:
#ifdef HAVE_LIBAUDIT
 if (!we_were_root)
   {
     /* should always work, if it did above */
     prctl (PR_SET_KEEPCAPS, 0, 0, 0, 0);
     cap_free (new_caps);
   }
#endif

 return FALSE;
}

/** Installs a UNIX signal handler
 *
 * @param sig the signal to handle
 * @param handler the handler
 */
void
_dbus_set_signal_handler (int               sig,
                          DBusSignalHandler handler)
{
  struct sigaction act;
  sigset_t empty_mask;
  
  sigemptyset (&empty_mask);
  act.sa_handler = handler;
  act.sa_mask    = empty_mask;
  act.sa_flags   = 0;
  sigaction (sig,  &act, NULL);
}


/**
 * Removes a directory; Directory must be empty
 * 
 * @param filename directory filename
 * @param error initialized error object
 * @returns #TRUE on success
 */
dbus_bool_t
_dbus_delete_directory (const DBusString *filename,
			DBusError        *error)
{
  const char *filename_c;
  
  _DBUS_ASSERT_ERROR_IS_CLEAR (error);

  filename_c = _dbus_string_get_const_data (filename);

  if (rmdir (filename_c) != 0)
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
		      "Failed to remove directory %s: %s\n",
		      filename_c, _dbus_strerror (errno));
      return FALSE;
    }
  
  return TRUE;
}

/** Checks if a file exists
*
* @param file full path to the file
* @returns #TRUE if file exists
*/
dbus_bool_t 
_dbus_file_exists (const char *file)
{
  return (access (file, F_OK) == 0);
}

/** Checks if user is at the console
*
* @param username user to check
* @param error return location for errors
* @returns #TRUE is the user is at the consolei and there are no errors
*/
dbus_bool_t 
_dbus_user_at_console (const char *username,
                       DBusError  *error)
{

  DBusString f;
  dbus_bool_t result;

  result = FALSE;
  if (!_dbus_string_init (&f))
    {
      _DBUS_SET_OOM (error);
      return FALSE;
    }

  if (!_dbus_string_append (&f, DBUS_CONSOLE_AUTH_DIR))
    {
      _DBUS_SET_OOM (error);
      goto out;
    }


  if (!_dbus_string_append (&f, username))
    {
      _DBUS_SET_OOM (error);
      goto out;
    }

  result = _dbus_file_exists (_dbus_string_get_const_data (&f));

 out:
  _dbus_string_free (&f);

  return result;
}


/**
 * Checks whether the filename is an absolute path
 *
 * @param filename the filename
 * @returns #TRUE if an absolute path
 */
dbus_bool_t
_dbus_path_is_absolute (const DBusString *filename)
{
  if (_dbus_string_get_length (filename) > 0)
    return _dbus_string_get_byte (filename, 0) == '/';
  else
    return FALSE;
}

/**
 * stat() wrapper.
 *
 * @param filename the filename to stat
 * @param statbuf the stat info to fill in
 * @param error return location for error
 * @returns #FALSE if error was set
 */
dbus_bool_t
_dbus_stat (const DBusString *filename,
            DBusStat         *statbuf,
            DBusError        *error)
{
  const char *filename_c;
  struct stat sb;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  filename_c = _dbus_string_get_const_data (filename);

  if (stat (filename_c, &sb) < 0)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "%s", _dbus_strerror (errno));
      return FALSE;
    }

  statbuf->mode = sb.st_mode;
  statbuf->nlink = sb.st_nlink;
  statbuf->uid = sb.st_uid;
  statbuf->gid = sb.st_gid;
  statbuf->size = sb.st_size;
  statbuf->atime = sb.st_atime;
  statbuf->mtime = sb.st_mtime;
  statbuf->ctime = sb.st_ctime;

  return TRUE;
}


/**
 * Internals of directory iterator
 */
struct DBusDirIter
{
  DIR *d; /**< The DIR* from opendir() */
  
};

/**
 * Open a directory to iterate over.
 *
 * @param filename the directory name
 * @param error exception return object or #NULL
 * @returns new iterator, or #NULL on error
 */
DBusDirIter*
_dbus_directory_open (const DBusString *filename,
                      DBusError        *error)
{
  DIR *d;
  DBusDirIter *iter;
  const char *filename_c;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
  
  filename_c = _dbus_string_get_const_data (filename);

  d = opendir (filename_c);
  if (d == NULL)
    {
      dbus_set_error (error, _dbus_error_from_errno (errno),
                      "Failed to read directory \"%s\": %s",
                      filename_c,
                      _dbus_strerror (errno));
      return NULL;
    }
  iter = dbus_new0 (DBusDirIter, 1);
  if (iter == NULL)
    {
      closedir (d);
      dbus_set_error (error, DBUS_ERROR_NO_MEMORY,
                      "Could not allocate memory for directory iterator");
      return NULL;
    }

  iter->d = d;

  return iter;
}

/* Calculate the required buffer size (in bytes) for directory
 * entries read from the given directory handle.  Return -1 if this
 * this cannot be done. 
 *
 * If you use autoconf, include fpathconf and dirfd in your
 * AC_CHECK_FUNCS list.  Otherwise use some other method to detect
 * and use them where available.
 */
static dbus_bool_t
dirent_buf_size(DIR * dirp, size_t *size)
{
 long name_max;
#   if defined(HAVE_FPATHCONF) && defined(_PC_NAME_MAX)
#      if defined(HAVE_DIRFD)
          name_max = fpathconf(dirfd(dirp), _PC_NAME_MAX);
#      elif defined(HAVE_DDFD)
          name_max = fpathconf(dirp->dd_fd, _PC_NAME_MAX);
#      else
          name_max = fpathconf(dirp->__dd_fd, _PC_NAME_MAX);
#      endif /* HAVE_DIRFD */
     if (name_max == -1)
#           if defined(NAME_MAX)
	     name_max = NAME_MAX;
#           else
	     return FALSE;
#           endif
#   elif defined(MAXNAMELEN)
     name_max = MAXNAMELEN;
#   else
#       if defined(NAME_MAX)
	 name_max = NAME_MAX;
#       else
#           error "buffer size for readdir_r cannot be determined"
#       endif
#   endif
  if (size)
    *size = (size_t)offsetof(struct dirent, d_name) + name_max + 1;
  else
    return FALSE;

  return TRUE;
}

/**
 * Get next file in the directory. Will not return "." or ".."  on
 * UNIX. If an error occurs, the contents of "filename" are
 * undefined. The error is never set if the function succeeds.
 *
 * @param iter the iterator
 * @param filename string to be set to the next file in the dir
 * @param error return location for error
 * @returns #TRUE if filename was filled in with a new filename
 */
dbus_bool_t
_dbus_directory_get_next_file (DBusDirIter      *iter,
                               DBusString       *filename,
                               DBusError        *error)
{
  struct dirent *d, *ent;
  size_t buf_size;
  int err;

  _DBUS_ASSERT_ERROR_IS_CLEAR (error);
 
  if (!dirent_buf_size (iter->d, &buf_size))
    {
      dbus_set_error (error, DBUS_ERROR_FAILED,
                      "Can't calculate buffer size when reading directory");
      return FALSE;
    }

  d = (struct dirent *)dbus_malloc (buf_size);
  if (!d)
    {
      dbus_set_error (error, DBUS_ERROR_NO_MEMORY,
                      "No memory to read directory entry");
      return FALSE;
    }

 again:
  err = readdir_r (iter->d, d, &ent);
  if (err || !ent)
    {
      if (err != 0)
        dbus_set_error (error,
                        _dbus_error_from_errno (err),
                        "%s", _dbus_strerror (err));

      dbus_free (d);
      return FALSE;
    }
  else if (ent->d_name[0] == '.' &&
           (ent->d_name[1] == '\0' ||
            (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
    goto again;
  else
    {
      _dbus_string_set_length (filename, 0);
      if (!_dbus_string_append (filename, ent->d_name))
        {
          dbus_set_error (error, DBUS_ERROR_NO_MEMORY,
                          "No memory to read directory entry");
          dbus_free (d);
          return FALSE;
        }
      else
        {
          dbus_free (d);
          return TRUE;
        }
    }
}

/**
 * Closes a directory iteration.
 */
void
_dbus_directory_close (DBusDirIter *iter)
{
  closedir (iter->d);
  dbus_free (iter);
}

static dbus_bool_t
fill_user_info_from_group (struct group  *g,
                           DBusGroupInfo *info,
                           DBusError     *error)
{
  _dbus_assert (g->gr_name != NULL);
  
  info->gid = g->gr_gid;
  info->groupname = _dbus_strdup (g->gr_name);

  /* info->members = dbus_strdupv (g->gr_mem) */
  
  if (info->groupname == NULL)
    {
      dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL);
      return FALSE;
    }

  return TRUE;
}

static dbus_bool_t
fill_group_info (DBusGroupInfo    *info,
                 dbus_gid_t        gid,
                 const DBusString *groupname,
                 DBusError        *error)
{
  const char *group_c_str;

  _dbus_assert (groupname != NULL || gid != DBUS_GID_UNSET);
  _dbus_assert (groupname == NULL || gid == DBUS_GID_UNSET);

  if (groupname)
    group_c_str = _dbus_string_get_const_data (groupname);
  else
    group_c_str = NULL;
  
  /* For now assuming that the getgrnam() and getgrgid() flavors
   * always correspond to the pwnam flavors, if not we have
   * to add more configure checks.
   */
  
#if defined (HAVE_POSIX_GETPWNAM_R) || defined (HAVE_NONPOSIX_GETPWNAM_R)
  {
    struct group *g;
    int result;
    size_t buflen;
    char *buf;
    struct group g_str;
    dbus_bool_t b;

    /* retrieve maximum needed size for buf */
    buflen = sysconf (_SC_GETGR_R_SIZE_MAX);

    if (buflen <= 0)
      buflen = 1024;

    result = -1;
    while (1)
      {
        buf = dbus_malloc (buflen);
        if (buf == NULL)
          {
            dbus_set_error (error, DBUS_ERROR_NO_MEMORY, NULL);
            return FALSE;
          }

        g = NULL;
#ifdef HAVE_POSIX_GETPWNAM_R
        if (group_c_str)
          result = getgrnam_r (group_c_str, &g_str, buf, buflen,
                               &g);
        else
          result = getgrgid_r (gid, &g_str, buf, buflen,
                               &g);
#else
        g = getgrnam_r (group_c_str, &g_str, buf, buflen);
        result = 0;
#endif /* !HAVE_POSIX_GETPWNAM_R */
        /* Try a bigger buffer if ERANGE was returned:
           https://bugs.freedesktop.org/show_bug.cgi?id=16727
        */
        if (result == ERANGE && buflen < 512 * 1024)
          {
            dbus_free (buf);
            buflen *= 2;
          }
        else
          {
            break;
          }
      }

    if (result == 0 && g == &g_str)
      {
        b = fill_user_info_from_group (g, info, error);
        dbus_free (buf);
        return b;
      }
    else
      {
        dbus_set_error (error, _dbus_error_from_errno (errno),
                        "Group %s unknown or failed to look it up\n",
                        group_c_str ? group_c_str : "???");
        dbus_free (buf);
        return FALSE;
      }
  }
#else /* ! HAVE_GETPWNAM_R */
  {
    /* I guess we're screwed on thread safety here */
    struct group *g;

    g = getgrnam (group_c_str);

    if (g != NULL)
      {
        return fill_user_info_from_group (g, info, error);
      }
    else
      {
        dbus_set_error (error, _dbus_error_from_errno (errno),
                        "Group %s unknown or failed to look it up\n",
                        group_c_str ? group_c_str : "???");
        return FALSE;
      }
  }
#endif  /* ! HAVE_GETPWNAM_R */
}

/**
 * Initializes the given DBusGroupInfo struct
 * with information about the given group name.
 *
 * @param info the group info struct
 * @param groupname name of group
 * @param error the error return
 * @returns #FALSE if error is set
 */
dbus_bool_t
_dbus_group_info_fill (DBusGroupInfo    *info,
                       const DBusString *groupname,
                       DBusError        *error)
{
  return fill_group_info (info, DBUS_GID_UNSET,
                          groupname, error);

}

/**
 * Initializes the given DBusGroupInfo struct
 * with information about the given group ID.
 *
 * @param info the group info struct
 * @param gid group ID
 * @param error the error return
 * @returns #FALSE if error is set
 */
dbus_bool_t
_dbus_group_info_fill_gid (DBusGroupInfo *info,
                           dbus_gid_t     gid,
                           DBusError     *error)
{
  return fill_group_info (info, gid, NULL, error);
}

/**
 * Parse a UNIX user from the bus config file. On Windows, this should
 * simply always fail (just return #FALSE).
 *
 * @param username the username text
 * @param uid_p place to return the uid
 * @returns #TRUE on success
 */
dbus_bool_t
_dbus_parse_unix_user_from_config (const DBusString  *username,
                                   dbus_uid_t        *uid_p)
{
  return _dbus_get_user_id (username, uid_p);

}

/**
 * Parse a UNIX group from the bus config file. On Windows, this should
 * simply always fail (just return #FALSE).
 *
 * @param groupname the groupname text
 * @param gid_p place to return the gid
 * @returns #TRUE on success
 */
dbus_bool_t
_dbus_parse_unix_group_from_config (const DBusString  *groupname,
                                    dbus_gid_t        *gid_p)
{
  return _dbus_get_group_id (groupname, gid_p);
}

/**
 * Gets all groups corresponding to the given UNIX user ID. On UNIX,
 * just calls _dbus_groups_from_uid(). On Windows, should always
 * fail since we don't know any UNIX groups.
 *
 * @param uid the UID
 * @param group_ids return location for array of group IDs
 * @param n_group_ids return location for length of returned array
 * @returns #TRUE if the UID existed and we got some credentials
 */
dbus_bool_t
_dbus_unix_groups_from_uid (dbus_uid_t            uid,
                            dbus_gid_t          **group_ids,
                            int                  *n_group_ids)
{
  return _dbus_groups_from_uid (uid, group_ids, n_group_ids);
}

/**
 * Checks to see if the UNIX user ID is at the console.
 * Should always fail on Windows (set the error to
 * #DBUS_ERROR_NOT_SUPPORTED).
 *
 * @param uid UID of person to check 
 * @param error return location for errors
 * @returns #TRUE if the UID is the same as the console user and there are no errors
 */
dbus_bool_t
_dbus_unix_user_is_at_console (dbus_uid_t         uid,
                               DBusError         *error)
{
  return _dbus_is_console_user (uid, error);

}

/**
 * Checks to see if the UNIX user ID matches the UID of
 * the process. Should always return #FALSE on Windows.
 *
 * @param uid the UNIX user ID
 * @returns #TRUE if this uid owns the process.
 */
dbus_bool_t
_dbus_unix_user_is_process_owner (dbus_uid_t uid)
{
  return uid == _dbus_geteuid ();
}

/**
 * Checks to see if the Windows user SID matches the owner of
 * the process. Should always return #FALSE on UNIX.
 *
 * @param windows_sid the Windows user SID
 * @returns #TRUE if this user owns the process.
 */
dbus_bool_t
_dbus_windows_user_is_process_owner (const char *windows_sid)
{
  return FALSE;
}

/** @} */ /* End of DBusInternalsUtils functions */

/**
 * @addtogroup DBusString
 *
 * @{
 */
/**
 * Get the directory name from a complete filename
 * @param filename the filename
 * @param dirname string to append directory name to
 * @returns #FALSE if no memory
 */
dbus_bool_t
_dbus_string_get_dirname  (const DBusString *filename,
                           DBusString       *dirname)
{
  int sep;
  
  _dbus_assert (filename != dirname);
  _dbus_assert (filename != NULL);
  _dbus_assert (dirname != NULL);

  /* Ignore any separators on the end */
  sep = _dbus_string_get_length (filename);
  if (sep == 0)
    return _dbus_string_append (dirname, "."); /* empty string passed in */
    
  while (sep > 0 && _dbus_string_get_byte (filename, sep - 1) == '/')
    --sep;

  _dbus_assert (sep >= 0);
  
  if (sep == 0)
    return _dbus_string_append (dirname, "/");
  
  /* Now find the previous separator */
  _dbus_string_find_byte_backward (filename, sep, '/', &sep);
  if (sep < 0)
    return _dbus_string_append (dirname, ".");
  
  /* skip multiple separators */
  while (sep > 0 && _dbus_string_get_byte (filename, sep - 1) == '/')
    --sep;

  _dbus_assert (sep >= 0);
  
  if (sep == 0 &&
      _dbus_string_get_byte (filename, 0) == '/')
    return _dbus_string_append (dirname, "/");
  else
    return _dbus_string_copy_len (filename, 0, sep - 0,
                                  dirname, _dbus_string_get_length (dirname));
}
/** @} */ /* DBusString stuff */

