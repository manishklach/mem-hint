# Patent Pending: Indian Patent Application No. 202641053160
# Inventor: Manish KL, filed 26 April 2026

from mem_hint import MemHintScheduler


class WrappedFSDP:
    """Illustrative wrapper around a training module."""

    def __init__(self, module, scheduler: MemHintScheduler):
        self.module = module
        self.scheduler = scheduler

    def forward(self, batch):
        self.scheduler.on_forward_pass(batch_size=len(batch))
        return self.module.forward(batch)

    def backward(self, loss, step: int):
        self.scheduler.on_backward_pass(step=step)
        loss.backward()
        # A production implementation could mirror the phase hint to GPU-side
        # firmware through shared-memory IPC or doorbells for tighter coupling.


class DummyLoss:
    def backward(self):
        print("backward() called")


class DummyModule:
    def forward(self, batch):
        print(f"forward(batch={batch})")
        return DummyLoss()


def main():
    with MemHintScheduler() as scheduler:
        model = WrappedFSDP(DummyModule(), scheduler)
        optimizer_steps = 2
        for step in range(optimizer_steps):
            loss = model.forward(["sample-a", "sample-b"])
            model.backward(loss, step=step)
            print(f"optimizer step {step}")
        scheduler.on_idle()


if __name__ == "__main__":
    main()
