# Patent Pending: Indian Patent Application No. 202641053160

import argparse
import os
import sys
import time

from mem_hint import MemHintClient
from mem_hint import PHASE_AGENTIC
from mem_hint import PHASE_BACKWARD
from mem_hint import PHASE_DECODE
from mem_hint import PHASE_FORWARD
from mem_hint import PHASE_IDLE
from mem_hint import PHASE_PREFILL

PHASE_NAME_TO_ID = {
    "prefill": PHASE_PREFILL,
    "decode": PHASE_DECODE,
    "agentic": PHASE_AGENTIC,
    "idle": PHASE_IDLE,
    "forward": PHASE_FORWARD,
    "backward": PHASE_BACKWARD,
}

STATUS_ATTRS = [
    "current_phase",
    "p99_latency_ns",
    "ecc_correctable_rate",
    "ecc_uncorrectable_count",
    "read_retry_count",
    "last_transition_ms",
    "active_channel",
]


def _open_client():
    client = MemHintClient()
    try:
        client.__enter__()
    except FileNotFoundError:
        print("mem_hint device not found: /dev/mem_hint", file=sys.stderr)
        return None
    except PermissionError:
        print(
            "permission denied opening /dev/mem_hint; "
            "try with sufficient privileges",
            file=sys.stderr,
        )
        return None
    except OSError as exc:
        print(f"failed to open /dev/mem_hint: {exc}", file=sys.stderr)
        return None
    return client


def cmd_send(args):
    client = _open_client()
    if client is None:
        return 1
    try:
        client.send(
            phase_id=PHASE_NAME_TO_ID[args.phase],
            latency_ns=args.latency,
            bw_gbps=args.bandwidth,
            security=args.security,
            priority=args.priority,
        )
        print(
            f"sent phase={args.phase} latency_ns={args.latency} "
            f"bw_gbps={args.bandwidth} priority={args.priority}"
        )
    except OSError as exc:
        print(f"failed to send hint: {exc}", file=sys.stderr)
        return 1
    finally:
        client.__exit__(None, None, None)
    return 0


def cmd_status(_args):
    for attr in STATUS_ATTRS:
        path = f"/sys/bus/platform/drivers/mem_hint/status/{attr}"
        if not os.path.exists(path):
            print(
                "mem_hint status interface not found under sysfs",
                file=sys.stderr,
            )
            return 1
        with open(path, "r", encoding="utf-8") as handle:
            print(f"{attr}: {handle.read().strip()}")
    return 0


def cmd_monitor(args):
    print("phase | p99_latency_ns | ecc_correctable_rate | last_transition_ms")
    try:
        while True:
            values = {}
            for attr in [
                "current_phase",
                "p99_latency_ns",
                "ecc_correctable_rate",
                "last_transition_ms",
            ]:
                path = f"/sys/bus/platform/drivers/mem_hint/status/{attr}"
                if not os.path.exists(path):
                    print(
                        "mem_hint status interface not found under sysfs",
                        file=sys.stderr,
                    )
                    return 1
                with open(path, "r", encoding="utf-8") as handle:
                    values[attr] = handle.read().strip()
            print(
                f"{values['current_phase']} | {values['p99_latency_ns']} | "
                f"{values['ecc_correctable_rate']} | "
                f"{values['last_transition_ms']}"
            )
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nmonitor stopped")
        return 0


def build_parser():
    parser = argparse.ArgumentParser(description="mem_hint reference CLI")
    subparsers = parser.add_subparsers(dest="command", required=True)

    send_parser = subparsers.add_parser("send", help="send a workload hint")
    send_parser.add_argument(
        "--phase",
        choices=sorted(PHASE_NAME_TO_ID.keys()),
        required=True,
        help="phase name",
    )
    send_parser.add_argument("--latency", type=int, default=0)
    send_parser.add_argument("--bandwidth", type=int, default=0)
    send_parser.add_argument("--security", type=int, default=0)
    send_parser.add_argument("--priority", type=int, default=7)
    send_parser.set_defaults(func=cmd_send)

    status_parser = subparsers.add_parser("status", help="show sysfs status")
    status_parser.set_defaults(func=cmd_status)

    monitor_parser = subparsers.add_parser(
        "monitor",
        help="monitor sysfs status",
    )
    monitor_parser.add_argument(
        "--interval",
        type=float,
        default=1.0,
    )
    monitor_parser.set_defaults(func=cmd_monitor)

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
