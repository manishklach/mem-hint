/*
 * mem_hint.c - C helper library for /dev/mem_hint
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation for research and interoperability discussion.
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../include/mem_hint.h"

#define MEM_HINT_DEVICE "/dev/mem_hint"
#define MEM_HINT_SYSFS_BASE "/sys/bus/platform/drivers/mem_hint"

static int mem_hint_send_phase(mem_hint_fd_t fd, uint8_t phase_id,
			       uint16_t latency_ns, uint16_t bw_gbps,
			       uint8_t security_level, uint8_t priority)
{
	struct mem_workload_hint hint;

	memset(&hint, 0, sizeof(hint));
	hint.phase_id = phase_id;
	hint.latency_target_ns = latency_ns;
	hint.bw_target_gbps = bw_gbps;
	hint.security_level = security_level;
	hint.priority = priority;

	return mem_hint_send(fd, &hint);
}

mem_hint_fd_t mem_hint_open(void)
{
	int fd;

	fd = open(MEM_HINT_DEVICE, O_WRONLY);
	if (fd < 0)
		return -errno;

	return fd;
}

void mem_hint_close(mem_hint_fd_t fd)
{
	if (fd >= 0)
		(void)close(fd);
}

int mem_hint_send(mem_hint_fd_t fd, const struct mem_workload_hint *hint)
{
	ssize_t written;

	if (fd < 0 || !hint)
		return -EINVAL;
	if (hint->phase_id < PHASE_PREFILL || hint->phase_id > PHASE_BACKWARD)
		return -EINVAL;

	written = write(fd, hint, sizeof(*hint));
	if (written < 0)
		return -errno;
	if ((size_t)written != sizeof(*hint))
		return -EIO;

	return 0;
}

int mem_hint_prefill(mem_hint_fd_t fd, uint16_t bw_gbps, uint8_t priority)
{
	return mem_hint_send_phase(fd, PHASE_PREFILL, 0, bw_gbps, 0, priority);
}

int mem_hint_decode(mem_hint_fd_t fd, uint16_t latency_ns, uint8_t priority)
{
	return mem_hint_send_phase(fd, PHASE_DECODE, latency_ns, 150, 0, priority);
}

int mem_hint_agentic(mem_hint_fd_t fd, uint8_t priority)
{
	return mem_hint_send_phase(fd, PHASE_AGENTIC, 0, 0, 0, priority);
}

int mem_hint_idle(mem_hint_fd_t fd)
{
	return mem_hint_send_phase(fd, PHASE_IDLE, 0, 0, 0, 3);
}

int mem_hint_forward_pass(mem_hint_fd_t fd, uint16_t bw_gbps)
{
	return mem_hint_send_phase(fd, PHASE_FORWARD, 0, bw_gbps, 0, 7);
}

int mem_hint_backward_pass(mem_hint_fd_t fd)
{
	return mem_hint_send_phase(fd, PHASE_BACKWARD, 0, 350, 0, 7);
}

int mem_hint_read_status(const char *attr, char *buf, size_t len)
{
	char path[512];
	FILE *fp;

	if (!attr || !buf || len == 0)
		return -EINVAL;

	if (snprintf(path, sizeof(path), "%s/status/%s",
		     MEM_HINT_SYSFS_BASE, attr) >= (int)sizeof(path))
		return -ENAMETOOLONG;

	fp = fopen(path, "r");
	if (!fp)
		return -errno;

	if (!fgets(buf, (int)len, fp)) {
		int ret = ferror(fp) ? -EIO : -ENODATA;
		fclose(fp);
		return ret;
	}

	fclose(fp);
	return 0;
}

int mem_hint_set_policy(const char *attr, uint64_t value)
{
	char path[512];
	FILE *fp;

	if (!attr)
		return -EINVAL;

	if (snprintf(path, sizeof(path), "%s/policy/%s",
		     MEM_HINT_SYSFS_BASE, attr) >= (int)sizeof(path))
		return -ENAMETOOLONG;

	fp = fopen(path, "w");
	if (!fp)
		return -errno;

	if (fprintf(fp, "%" PRIu64 "\n", value) < 0) {
		int ret = -errno;
		fclose(fp);
		return ret;
	}

	if (fclose(fp) != 0)
		return -errno;

	return 0;
}
