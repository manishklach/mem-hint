/*
 * mem_hint.h — userspace header for /dev/mem_hint
 * Patent Pending: Indian Patent Application No. 202641053160
 * Inventor: Manish KL, filed 26 April 2026
 * Reference implementation — illustrative, not production
 * silicon register definitions.
 */

#ifndef MEM_HINT_USERSPACE_H
#define MEM_HINT_USERSPACE_H

#include <stddef.h>
#include <stdint.h>

/* Phase ID constants */
#define PHASE_PREFILL    0x01
#define PHASE_DECODE     0x02
#define PHASE_AGENTIC    0x03
#define PHASE_IDLE       0x04
#define PHASE_FORWARD    0x05
#define PHASE_BACKWARD   0x06

/* Packed hint struct — 8 bytes exactly */
struct mem_workload_hint {
	uint8_t  phase_id;
	uint16_t latency_target_ns;
	uint16_t bw_target_gbps;
	uint8_t  security_level;
	uint8_t  priority;
	uint8_t  reserved;
} __attribute__((packed));

/* Library API */
int  mem_hint_open(const char *device);
void mem_hint_close(int fd);
int  mem_hint_send(int fd, const struct mem_workload_hint *h);
int  mem_hint_prefill(int fd, uint16_t bw_gbps, uint8_t priority);
int  mem_hint_decode(int fd, uint16_t latency_ns, uint8_t priority);
int  mem_hint_agentic(int fd, uint8_t priority);
int  mem_hint_idle(int fd);
int  mem_hint_forward_pass(int fd, uint16_t bw_gbps);
int  mem_hint_backward_pass(int fd);
int  mem_hint_read_status(const char *attr, char *buf, size_t len);
int  mem_hint_set_policy(const char *attr, uint64_t value);

#endif /* MEM_HINT_USERSPACE_H */
