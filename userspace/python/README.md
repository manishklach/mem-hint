# mem-hint Python Client

Python interface to the `/dev/mem_hint` kernel driver.
Patent Pending: Indian Patent Application No. 202641053160

## Install
    pip install -e .

## Quick start
    from mem_hint import MemHintClient, MemHintScheduler

    # Direct client
    with MemHintClient() as c:
        c.prefill(bw_gbps=400)
        c.decode(latency_ns=90)
        c.idle()

    # High-level scheduler
    with MemHintScheduler() as s:
        s.on_prefill_start(batch_size=32)
        s.on_decode_start(request_id="req-001")
        s.on_idle()

    # Dry-run (no kernel module needed)
    with MemHintClient(dry_run=True) as c:
        c.decode()   # logs only, no /dev/mem_hint required

## Phases
    from mem_hint.phases import PHASES, PHASE_DECODE
    print(PHASES[PHASE_DECODE])
