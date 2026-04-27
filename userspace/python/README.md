# mem-hint Python Client

Python interface to `/dev/mem_hint`.
Patent Pending: Indian Patent Application No. 202641053160

## Install
    pip install -e .

## Quick start
    from mem_hint import MemHintClient, MemHintScheduler

    # Direct — needs kernel module loaded
    with MemHintClient() as c:
        c.prefill(bw_gbps=400)
        c.decode(latency_ns=90)
        c.idle()

    # Dry-run — no kernel module needed (for CI / testing)
    with MemHintClient(dry_run=True) as c:
        c.decode()

    # High-level scheduler
    with MemHintScheduler() as s:
        s.on_prefill_start(batch_size=32)
        s.on_decode_start("req-001")
        s.on_idle()

## Phases
    from mem_hint.phases import PHASES, PHASE_DECODE
    print(PHASES[PHASE_DECODE])
    # Phase(id=2, name='Decode', ...)
