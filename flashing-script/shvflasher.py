#!/usr/bin/env python3

# Copyright: Michal Lenc 2025 <michallenc@seznam.cz>
#            Stepan Pressl 20025 <pressl.stepan@gmail.com>
import asyncio
import io

from shv import RpcUrl, SHVBytes, ValueClient


async def shv_flasher(connection: str, name: str, path_to_root: str, queue: asyncio.Queue | None) -> None:
    url = RpcUrl.parse(connection)
    client = await ValueClient.connect(url)
    assert client is not None
    node_name = f"{path_to_root}/fwUpdate" 

    res = await client.call(node_name, "stat")

    maxwrite = res[5]
    print(f"Received maximum enabled write size {maxwrite}.")
    
    print(f"Started uploading new firmware {name}... this may take some time.")
    with open(name, mode="rb") as f:
        if queue:
            f.seek(0, io.SEEK_END)
            size = f.tell()
            f.seek(0, io.SEEK_SET)
            transfers = size / maxwrite

        i = 0
        while data := f.read(maxwrite):
            offset = i * maxwrite
            res = await client.call(node_name, "write", [offset, SHVBytes(data)])
            i += 1
            if queue:
                currProgress = (int)((i * 100) / transfers)
                queue.put_nowait(currProgress)

    print("Flashing completed!")
    await client.disconnect()
