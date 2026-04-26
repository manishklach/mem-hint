/*
 * basic_hint.c - minimal /dev/mem_hint example
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation for research and interoperability discussion.
 *
 * Build with: gcc -O2 -o basic_hint basic_hint.c
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/mem_hint.h"

int main(void)
{
	struct mem_workload_hint hint;
	char status[64];
	FILE *fp;
	int fd;
	ssize_t written;

	memset(&hint, 0, sizeof(hint));
	hint.phase_id = PHASE_DECODE;
	hint.latency_target_ns = 90;
	hint.bw_target_gbps = 150;
	hint.priority = 7;

	fd = open("/dev/mem_hint", O_WRONLY);
	if (fd < 0) {
		perror("open(/dev/mem_hint)");
		return EXIT_FAILURE;
	}

	written = write(fd, &hint, sizeof(hint));
	if (written < 0 || (size_t)written != sizeof(hint)) {
		perror("write(mem_hint)");
		close(fd);
		return EXIT_FAILURE;
	}
	close(fd);

	fp = fopen("/sys/bus/platform/drivers/mem_hint/status/current_phase", "r");
	if (!fp) {
		perror("fopen(current_phase)");
		return EXIT_FAILURE;
	}

	if (!fgets(status, sizeof(status), fp)) {
		fprintf(stderr, "failed to read current_phase: %s\n",
			strerror(errno));
		fclose(fp);
		return EXIT_FAILURE;
	}
	fclose(fp);

	status[strcspn(status, "\n")] = '\0';
	printf("decode hint sent successfully, current phase=%s\n", status);
	return EXIT_SUCCESS;
}
