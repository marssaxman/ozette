/**
 * Copyright 2009-2010 Bart Trojanowski <bart@jukie.net>
 * Licensed under GPLv2, or later, at your choosing.
 *
 * bidirectional popen() call
 *
 * @param rwepipe - int array of size three
 * @param exe - program to run
 * @param argv - argument list
 * @return pid or -1 on error
 *
 * The caller passes in an array of three integers (rwepipe), on successful
 * execution it can then write to element 0 (stdin of exe), and read from
 * element 1 (stdout) and 2 (stderr).
 */

#ifndef CONSOLE_POPENRWE_H
#define CONSOLE_POPENRWE_H

int popenRWE(int *rwepipe, const char *exe, const char *const argv[]);
int pcloseRWE(int pid, int *rwepipe);

#endif CONSOLE_POPENRWE_H

