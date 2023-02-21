/*
 * Copyright (C) 2014 Teluu Inc. (http://www.teluu.com)
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
 */

/* A simple proxy application for gcc/g++ to remove '-mno-cygwin' option
 * when the gcc/g++ doesn't support it. Note that the option seems to be
 * deprecated and causing compile error since gcc 4.7, while the option is
 * auto-generated by Python distutils module (up to Python 2.7) in building
 * Python extension.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Find gcc executable from env var PATH but omitting current dir */
const char* find_gcc(const char *gcc_exe)
{
    static char fname[256];
    char spath[1024 * 10];
    char *p;

    p = getenv("PATH");
    if (strlen(p) >= sizeof(spath)) {
        printf("Error: find_gcc() not enough buffer 1\n");
        return NULL;
    }

    pj_ansi_strncpy(spath, p, sizeof(spath));
    p = strtok(spath, ";");
    while (p) {
        int len;

        /* Skip current dir */
        if (strcmp(p, ".") == 0) {
            p = strtok(NULL, ";");
            continue;
        }

        len = snprintf(fname, sizeof(fname), "%s\\%s", p, gcc_exe);
        if (len < 0 || len >= sizeof(fname)) {
            printf("Error: find_gcc() not enough buffer 2\n");
            return NULL;
        }

        if (access(fname, F_OK | X_OK) != -1) {
            return fname;
        }
        
        p = strtok(NULL, ";");
    }

    return NULL;
}

int check_gcc_reject_mno_cygwin(const char *gcc_path)
{
    FILE *fp;
    char tmp[1024];
    const char *p;
    int ver;

    snprintf(tmp, sizeof(tmp), "%s -mno-cygwin 2>&1", gcc_path);
    fp = popen(tmp, "r");
    if (fp == NULL) {
        printf("Error: failed to run gcc\n" );
        return -1;
    }

    while (fgets(tmp, sizeof(tmp), fp) != NULL) {
        if (strstr(tmp, "unrecognized") && strstr(tmp, "-mno-cygwin"))
            return 1;
    }

    pclose(fp);
    return 0;
}

int main(int argc, const char const **argv)
{
    const char *app = "python cc_mingw.py";
    char cmd[1024 * 8], *p;
    int i, len, sz;
    int ret = 0;
    const char *spath, *gcc_exe;
    int remove_mno_cygwin;

    if (strstr(argv[0], "gcc") || strstr(argv[0], "GCC")) {
        gcc_exe = "gcc.exe";
    } else if (strstr(argv[0], "g++") || strstr(argv[0], "G++")) {
        gcc_exe = "g++.exe";
    } else {
        printf("Error: app name not gcc/g++\n");
        return -10;
    }

    /* Resolve GCC path from PATH env var */
    gcc_exe = find_gcc(gcc_exe);
    if (!gcc_exe) {
        printf("Error: real gcc/g++ not found\n");
        return -20;
    }

    /* Check if GCC rejects '-mno-cygwin' option */
    remove_mno_cygwin = check_gcc_reject_mno_cygwin(gcc_exe);
    if (remove_mno_cygwin < 0)
        return -30;
    
    len = snprintf(cmd, sizeof(cmd), "%s", gcc_exe);
    p = cmd + len;
    sz = sizeof(cmd) - len;
    for (i = 1; i < argc && sz > 0; ++i) {

        if (remove_mno_cygwin && strcmp(argv[i], "-mno-cygwin") == 0) {
            printf("Removed option '-mno-cygwin'.\n");
            continue;
        }

        len = snprintf(p, sz, " %s", argv[i]);
        if (len < 0 || len >= sz) {
            ret = E2BIG;
            break;
        }
        p += len;
        sz -= len;
    }
    
    if (!ret) {
        //printf("cmd = %s\n", cmd);
        ret = system(cmd);
    }

    if (ret) {
        printf("Error: %s\n", strerror(ret));
    }
    
    return ret;
}
