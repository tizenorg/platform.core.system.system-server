#include <stdio.h>
#include <stdlib.h>

/**
 * Opens "/proc/$pid/oom_adj" file for w/r;
 * Return: FILE pointer or NULL
 */
FILE * open_proc_oom_adj_file(int pid, const char *mode)
{
        char buf[32];
        FILE *fp;

	/* snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", pid); */
	/*
	 * Warn that /proc/pid/oom_adj is deprecated, see
	 * Documentation/feature-removal-schedule.txt.
	 * Please use /proc/%d/oom_score_adj instead.
	 */
	snprintf(buf, sizeof(buf), "/proc/%d/oom_adj", pid);
	fp = fopen(buf, mode);
	return fp;
}

