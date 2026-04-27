/*
 * basic_hint.c — minimal /dev/mem_hint example
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation for research and interoperability discussion.
 *
 * Build with:
 *   cd userspace/lib && make && cd ../examples && make basic_hint
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem_hint.h"

int main(void)
{
	struct mem_workload_hint hint;
	char phase_buf[64];
	char latency_buf[64];
	int fd;
	int ret;

	/* Step 1: Open /dev/mem_hint */
	fd = mem_hint_open("/dev/mem_hint");
	if (fd < 0) {
		fprintf(stderr, "ERROR: mem_hint_open failed: %s\n",
			strerror(-fd));
		return 1;
	}

	/* Step 2: Send PHASE_DECODE hint */
	memset(&hint, 0, sizeof(hint));
	hint.phase_id         = PHASE_DECODE;   /* 0x02 */
	hint.latency_target_ns = 90;
	hint.bw_target_gbps   = 150;
	hint.priority         = 7;

	ret = mem_hint_send(fd, &hint);
	if (ret < 0) {
		fprintf(stderr, "ERROR: mem_hint_send failed: %s\n",
			strerror(-ret));
		mem_hint_close(fd);
		return 1;
	}
	printf("Decode hint sent: phase=0x%02x lat=%u bw=%u pri=%u\n",
	       hint.phase_id, hint.latency_target_ns,
	       hint.bw_target_gbps, hint.priority);

	/* Step 3: Read back current_phase from sysfs */
	ret = mem_hint_read_status("current_phase", phase_buf,
				   sizeof(phase_buf));
	if (ret < 0) {
		fprintf(stderr, "WARNING: could not read current_phase: %s\n",
			strerror(-ret));
	} else {
		phase_buf[strcspn(phase_buf, "\n")] = '\0';
		printf("  current_phase = %s\n", phase_buf);
	}

	/* Step 4: Read back p99_latency_ns */
	ret = mem_hint_read_status("p99_latency_ns", latency_buf,
				   sizeof(latency_buf));
	if (ret < 0) {
		fprintf(stderr, "WARNING: could not read p99_latency_ns: %s\n",
			strerror(-ret));
	} else {
		latency_buf[strcspn(latency_buf, "\n")] = '\0';
		printf("  p99_latency_ns = %s\n", latency_buf);
	}

	/* Step 5: Print formatted status */
	printf("\n--- Status ---\n");
	printf("Phase: %s | P99 latency: %s ns\n",
	       phase_buf[0] ? phase_buf : "N/A",
	       latency_buf[0] ? latency_buf : "N/A");

	/* Step 6: Close */
	mem_hint_close(fd);
	printf("Done.\n");

	return 0;
}
