# Patent Pending: Indian Patent Application No. 202641053160
# Inventor: Manish KL, filed 26 April 2026

import asyncio

from mem_hint import MemHintScheduler


class AsyncLLMEngineStub:
    """Illustrative vLLM-like engine with mem_hint integration points."""

    def __init__(self):
        self.pending = []

    async def generate(self, prompt: str):
        self.pending.append(prompt)
        await asyncio.sleep(0.01)
        return f"completion:{prompt}"

    async def _run_engine(self):
        for _ in range(3):
            await asyncio.sleep(0.005)


class MemHintAsyncLLMEngine(AsyncLLMEngineStub):
    def __init__(self, scheduler: MemHintScheduler):
        super().__init__()
        self.scheduler = scheduler

    async def generate(self, prompt: str):
        self.scheduler.on_prefill_start(batch_size=len(self.pending) + 1)
        result = await super().generate(prompt)
        if not self.pending:
            self.scheduler.on_idle()
        return result

    async def _run_engine(self):
        for token_step in range(3):
            _ = token_step
            self.scheduler.on_decode_start(request_id="req-1")
            await asyncio.sleep(0.005)
        self.pending.clear()
        self.scheduler.on_idle()


async def main():
    with MemHintScheduler() as scheduler:
        engine = MemHintAsyncLLMEngine(scheduler)
        print(await engine.generate("hello mem_hint"))
        await engine._run_engine()
        print("queue empty -> idle")


if __name__ == "__main__":
    asyncio.run(main())
