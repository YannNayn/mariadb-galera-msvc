/* Copyright (c) 2009 Sun Microsystems, Inc.
   Use is subject to license terms.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/* 
  main() for mysqld.
  Calls mysqld_main() entry point exported by sql library.
*/

#ifdef _MSC_VER
#include <windows.h>
#include <stdlib.h>
#endif

extern int mysqld_main(int argc, char **argv);

int main(int argc, char **argv)
{
#ifdef _MSC_VER
    const char *dbg_env=getenv("BRK_DBG_MARIADB_GALERA");
    if(dbg_env && (!stricmp(dbg_env,"1") || !stricmp(dbg_env,"TRUE") ))
        DebugBreak();
#endif
  return mysqld_main(argc, argv);
}
