/* vi: set sw=4 ts=4: */
/*
 * Copyright (C) 1999,2000 by Lineo, inc. and Erik Andersen
 * Copyright (C) 1999-2002 by Erik Andersen <andersee@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "tinylogin.h"

#ifdef L_full_version
	const char * const full_version = BB_BANNER " multi-call binary";
#endif

#ifdef L_name_too_long
	const char * const name_too_long = "file name too long";
#endif

#ifdef L_omitting_directory
	const char * const omitting_directory = "%s: omitting directory";
#endif

#ifdef L_not_a_directory
	const char * const not_a_directory = "%s: not a directory";
#endif

#ifdef L_memory_exhausted
	const char * const memory_exhausted = "memory exhausted";
#endif

#ifdef L_invalid_date
	const char * const invalid_date = "invalid date `%s'";
#endif

#ifdef L_invalid_option
	const char * const invalid_option = "invalid option -- %c";
#endif

#ifdef L_io_error
	const char * const io_error = "%s: input/output error -- %s";
#endif

#ifdef L_dash_dash_help
	const char * const dash_dash_help = "--help";
#endif

#ifdef L_write_error
	const char * const write_error = "Write Error";
#endif

#ifdef L_too_few_args
	const char * const too_few_args = "too few arguments";
#endif

#ifdef L_name_longer_than_foo
	const char * const name_longer_than_foo = "Names longer than %d chars not supported.";
#endif

#ifdef L_unknown
	const char * const unknown = "(unknown)";
#endif

#ifdef L_can_not_create_raw_socket
	const char * const can_not_create_raw_socket = "can`t create raw socket";
#endif

#ifdef L_nologin_file
#define NOLOGIN_FILE       "/etc/nologin"
const char * const nologin_file = NOLOGIN_FILE;
#endif

#ifdef L_passwd_file
#define PASSWD_FILE        "/etc/passwd"
const char * const passwd_file = PASSWD_FILE;
#endif

#ifdef L_shadow_file
#define SHADOW_FILE        "/etc/shadow"
const char * const shadow_file = SHADOW_FILE;
#endif

#ifdef L_gshadow_file
#define GSHADOW_FILE       "/etc/gshadow"
const char * const gshadow_file = GSHADOW_FILE;
#endif

#ifdef L_group_file
#define GROUP_FILE         "/etc/group"
const char * const group_file = GROUP_FILE;
#endif

#ifdef L_securetty_file
#define SECURETTY_FILE     "/etc/securetty"
const char * const securetty_file = SECURETTY_FILE;
#endif

#ifdef L_motd_file
#define MOTD_FILE          "/etc/motd"
const char * const motd_file = MOTD_FILE;
#endif

#ifdef L_issue_file
#define ISSUE_FILE         "/etc/issue"
const char * const issue_file = ISSUE_FILE;
#endif

#ifdef L__path_login
#define _PATH_LOGIN        "/bin/login"
const char * const _path_login = _PATH_LOGIN;
#endif



