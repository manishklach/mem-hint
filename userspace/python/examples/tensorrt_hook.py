# Patent Pending: Indian Patent Application No. 202641053160
# Inventor: Manish KL, filed 26 April 2026

import time

from mem_hint import MemHintScheduler


class Session:
    """TensorRT-LLM-like session stub."""

    def run(self, mode: str):
        time.sleep(0.001)
        return {"mode": mode, "tokens": 32}


class MemHintSession:
    def __init__(self, session: Session, scheduler: MemHintScheduler):
        self.session = session
        self.scheduler = scheduler

    def run(self):
        start = time.perf_counter_ns()
        self.scheduler.on_prefill_start(batch_size=1)
        prefill_end = time.perf_counter_ns()
        prefill_result = self.session.run("prefill")
        self.scheduler.on_decode_start(request_id="trt-req")
        decode_hint_end = time.perf_counter_ns()
        decode_result = self.session.run("decode")
        self.scheduler.on_idle()
        print(
            "hint overhead (illustrative):",
            prefill_end - start,
            "ns prefill,",
            decode_hint_end - prefill_end,
            "ns decode",
        )
        return prefill_result, decode_result


if __name__ == "__main__":
    with MemHintScheduler() as scheduler:
        wrapper = MemHintSession(Session(), scheduler)
        print(wrapper.run())
