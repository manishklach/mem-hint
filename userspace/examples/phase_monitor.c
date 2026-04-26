/*
 * phase_monitor.c - live sysfs monitor for /dev/mem_hint
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation for research and interoperability discussion.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static void sleep_100ms(void) { Sleep(100); }
#else
#include <time.h>
static void sleep_100ms(void)
{
	struct timespec ts = { 0, 100000000L };

	nanosleep(&ts, NULL);
}
#endif

#include "../include/mem_hint.h"

static volatile sig_atomic_t keep_running = 1;

static void on_signal(int signo)
{
	(void)signo;
	keep_running = 0;
}

static int trim_status(char *buf)
{
	size_t len;

	len = strlen(buf);
	while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
		buf[len - 1] = '\0';
		len--;
	}

	return 0;
}

int main(void)
{
	char phase[64];
	char latency[64];
	char correctable[64];
	char transition[64];
	int ret;

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	printf("\033[2J\033[H");
	while (keep_running) {
		ret = mem_hint_read_status("current_phase", phase, sizeof(phase));
		ret |= mem_hint_read_status("p99_latency_ns", latency,
					    sizeof(latency));
		ret |= mem_hint_read_status("ecc_correctable_rate", correctable,
					    sizeof(correctable));
		ret |= mem_hint_read_status("last_transition_ms", transition,
					    sizeof(transition));
		if (ret < 0) {
			fprintf(stderr, "failed to read mem_hint status: %d\n", ret);
			return EXIT_FAILURE;
		}

		trim_status(phase);
		trim_status(latency);
		trim_status(correctable);
		trim_status(transition);

		printf("\033[H");
		printf("current_phase | p99_latency_ns | ecc_correctable_rate | last_transition_ms\n");
		printf("------------- | -------------- | -------------------- | ------------------\n");
		printf("%13s | %14s | %20s | %18s\n",
		       phase, latency, correctable, transition);
		fflush(stdout);
		sleep_100ms();
	}

	printf("\nmonitor stopped\n");
	return EXIT_SUCCESS;
}
