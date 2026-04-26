/*
 * mem_hint.h - userspace header for /dev/mem_hint
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation for research and interoperability discussion.
 */

#ifndef MEM_HINT_USERSPACE_H
#define MEM_HINT_USERSPACE_H

#include <stddef.h>
#include <stdint.h>

struct mem_workload_hint {
	uint8_t phase_id;
	uint16_t latency_target_ns;
	uint16_t bw_target_gbps;
	uint8_t security_level;
	uint8_t priority;
	uint8_t reserved[2];
} __attribute__((packed));

#define PHASE_PREFILL  0x01
#define PHASE_DECODE   0x02
#define PHASE_AGENTIC  0x03
#define PHASE_IDLE     0x04
#define PHASE_FORWARD  0x05
#define PHASE_BACKWARD 0x06

typedef int mem_hint_fd_t;

mem_hint_fd_t mem_hint_open(void);
void mem_hint_close(mem_hint_fd_t fd);
int mem_hint_send(mem_hint_fd_t fd, const struct mem_workload_hint *hint);
int mem_hint_prefill(mem_hint_fd_t fd, uint16_t bw_gbps, uint8_t priority);
int mem_hint_decode(mem_hint_fd_t fd, uint16_t latency_ns, uint8_t priority);
int mem_hint_agentic(mem_hint_fd_t fd, uint8_t priority);
int mem_hint_idle(mem_hint_fd_t fd);
int mem_hint_forward_pass(mem_hint_fd_t fd, uint16_t bw_gbps);
int mem_hint_backward_pass(mem_hint_fd_t fd);
int mem_hint_read_status(const char *attr, char *buf, size_t len);
int mem_hint_set_policy(const char *attr, uint64_t value);

#endif /* MEM_HINT_USERSPACE_H */
