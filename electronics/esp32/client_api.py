import asyncio
from enum import Enum
from typing import Self
from collections import deque
from websockets import ClientConnection
from hamming_decode import *
import websockets
import struct


class Mode(Enum):
    BUF_WRITE = 0
    STREAM = 1


class MPU6050Data:
    def __init__(self, data, CHECKSUM: Checksum):
        self.CHECKSUM = CHECKSUM
        if CHECKSUM != Checksum.CORRUPT:
            self.timestamp = data[0]
            self.ax = data[1]
            self.ay = data[2]
            self.az = data[3]
            self.gx = data[4]
            self.gy = data[5]
            self.gz = data[6]

    def __getattribute__(self, name):
        if (
            object.__getattribute__(self, "CHECKSUM") == Checksum.CORRUPT
            and name != "CHECKSUM"
        ):
            raise AttributeError("Attempting to read corrupt data!")
        return object.__getattribute__(self, name)


class ESP32_socket_link:
    PACKET = struct.Struct("<I6h")

    def __init__(self, url: str, MODE: Mode = Mode.BUF_WRITE) -> None:
        self.url: str = url
        self._ws: ClientConnection | None = None

        self._MODE: Mode = MODE
        self._buffer: deque[MPU6050Data] | None = None
        if self._MODE == Mode.BUF_WRITE:
            self._buffer = deque(maxlen=100000)

        self._rx_task: asyncio.Task | None = None

    async def __aenter__(self) -> Self:
        self._ws = await websockets.connect(self.url)
        if self._MODE == Mode.BUF_WRITE:
            self._rx_task = asyncio.create_task(self._receive())
        return self

    async def __aexit__(self, exc_type, exc, exc_tb) -> None:
        if self._rx_task is not None:
            self._rx_task.cancel()
            try:
                await self._rx_task
            except asyncio.CancelledError:
                pass
            self._rx_task = None

        if exc_type is not None:
            print(f"ERROR: Connection failed with exception: {exc}")
            print(f"Trace: {exc_tb}")

        if self._ws is not None:
            await self._ws.close()
            self._ws = None

    async def _receive(self) -> None:
        assert self._ws is not None
        if self._MODE != Mode.BUF_WRITE:
            raise RuntimeError(
                f"ERROR: Incorrect access mode, use {Mode.BUF_WRITE.name} instead of {self._MODE.name}"
            )
        try:
            assert self._buffer is not None
            async for msg in self._ws:
                if isinstance(msg, bytes):
                    for offset in range(0, len(msg), self.PACKET.size):
                        if offset + self.PACKET.size > len(msg):
                            print(
                                f"partial packet at offset {offset}, frame len {len(msg)}"
                            )
                            break
                        self._buffer.append(self.unpack_mpu(msg))
                if isinstance(msg, str):
                    print(msg)
        except websockets.ConnectionClosed:
            pass

    async def stream(self):
        assert self._ws is not None
        if self._MODE != Mode.STREAM:
            raise RuntimeError(
                f"ERROR: Incorrect access mode, use {Mode.STREAM.name} instead of {self._MODE.name}"
            )
        async for msg in self._ws:
            if isinstance(msg, bytes):
                for offset in range(0, len(msg), self.PACKET.size):
                    if offset + self.PACKET.size > len(msg):
                        print(
                            f"partial packet at offset {offset}, frame len {len(msg)}"
                        )
                        break

                    yield self.unpack_mpu(msg)
            if isinstance(msg, str):
                print(msg)

    def unpack_mpu(self, msg):
        syndrome = calculate_error_syndrome(msg)
        if syndrome > 128:
            mpuData = MPU6050Data((), Checksum.CORRUPT)
        elif syndrome == 0:
            data = self.PACKET.unpack_from(decode(msg, syndrome))
            mpuData = MPU6050Data(data, Checksum.NO_ERR)
        else:
            data = self.PACKET.unpack_from(decode(msg, syndrome))
            mpuData = MPU6050Data(data, Checksum.BIT_ERR)

        return mpuData

    async def issue_command(self, cmd: str) -> bool:
        if self._ws is None:
            return False
        try:
            await self._ws.send(cmd)
            return True
        except websockets.ConnectionClosed:
            return False

    def read_data(self) -> list[MPU6050Data]:
        if self._MODE != Mode.BUF_WRITE:
            raise RuntimeError(
                f"ERROR: Incorrect access mode, use {Mode.BUF_WRITE.name} instead of {self._MODE.name}"
            )
        assert self._buffer is not None
        batch = list(self._buffer)
        self._buffer.clear()

        return batch
