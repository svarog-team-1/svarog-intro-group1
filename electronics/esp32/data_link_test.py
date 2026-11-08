import asyncio
import math
from client_api import ESP32_socket_link, MPU6050Data, Mode

# Expected slopes from MockMPU. These are the single source of truth:
# if the mock's formulas change, change these and the test tells you what
# the wire actually carries.
SLOPE_AX = 1.0
SLOPE_AY = 2.0
SLOPE_AZ = 0.5
SLOPE_GX = 0.1
SLOPE_GY = -0.2
SLOPE_GZ = 0.3

# Intercepts (used only for the optional absolute check, not deltas).
INTERCEPT_AZ = 9.81


class VerifyError(AssertionError):
    pass


def close(a: float, b: float) -> bool:
    return math.isclose(a, b, rel_tol=1e-6, abs_tol=1e-1)


def verify_deltas(pkts: list[MPU6050Data]) -> None:
    if len(pkts) < 2:
        raise VerifyError(f"need >=2 packets, got {len(pkts)}")

    checks = [
        ("ax", SLOPE_AX),
        ("ay", SLOPE_AY),
        ("az", SLOPE_AZ),
        ("gx", SLOPE_GX),
        ("gy", SLOPE_GY),
        ("gz", SLOPE_GZ),
    ]

    for i in range(1, len(pkts)):
        prev, cur = pkts[i - 1], pkts[i]
        for field, slope in checks:
            d = getattr(cur, field) - getattr(prev, field)
            if not close(d, slope):
                raise VerifyError(
                    f"i={i} field={field}: delta={d!r}, expected {slope!r} "
                    f"(prev={getattr(prev, field)!r}, cur={getattr(cur, field)!r})"
                )


def verify_timestamps(pkts: list[MPU6050Data], step_ms: int) -> None:
    step_us = step_ms * 1000
    for i in range(1, len(pkts)):
        dt = pkts[i].timestamp - pkts[i - 1].timestamp
        if dt != step_us:
            raise VerifyError(
                f"i={i}: dt={dt} µs, expected {step_us} µs ({step_ms} ms)"
            )


async def verify_stream_deltas(
    url: str,
    *,
    expected_samples: int = 100,
    step_ms: int = 2,
    buf_mode: bool = True,
) -> list[MPU6050Data]:
    if buf_mode:
        async with ESP32_socket_link(url, Mode.BUF_WRITE) as link:
            collected: list[MPU6050Data] = []
            loop = asyncio.get_event_loop()
            deadline = loop.time() + 10.0
            while len(collected) < expected_samples:
                if loop.time() > deadline:
                    raise VerifyError(
                        f"timeout: got {len(collected)}/{expected_samples} samples"
                    )
                collected.extend(link.read_data())
                await asyncio.sleep(0.01)  # 100 Hz poll; batches arrive at 50 Hz
    else:
        async with ESP32_socket_link(url, Mode.STREAM) as link:
            collected = []
            async for pkt in link.stream():
                collected.append(pkt)
                if len(collected) >= expected_samples:
                    break

    collected = collected[:expected_samples]

    if len(collected) != expected_samples:
        raise VerifyError(f"got {len(collected)}, expected {expected_samples}")

    verify_deltas(collected)
    verify_timestamps(collected, step_ms)

    print(f"OK: {len(collected)} packets, deltas and timestamps verified")

    return collected


async def main():
    await verify_stream_deltas("ws://192.168.4.1:400")


if __name__ == "__main__":
    asyncio.run(main())
