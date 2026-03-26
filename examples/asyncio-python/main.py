#!/usr/bin/env python3
# GPLv2
# Copyright 2026 by Alexander Couzens <lynxis@fe80.eu>

import asyncio
import argparse
import gi
from gi.events import GLibEventLoopPolicy

gi.require_version('Qmi', '1.0')
from gi.repository import GLib, Gio, Qmi

POLICY = GLibEventLoopPolicy()
asyncio.set_event_loop_policy(POLICY)
LOOP = POLICY.get_event_loop()

async def main(qmi_uri):
    open_flags = Qmi.DeviceOpenFlags.AUTO | Qmi.DeviceOpenFlags.EXPECT_INDICATIONS
    timeout = 3
    device = None

    if qmi_uri.startswith('/'):
        file = Gio.File.new_for_path(qmi_uri)
        device = await Qmi.Device.new(file)
    elif qmi_uri.startswith('qrtr://'):
        gi.require_version('Qrtr', '1.0')
        from gi.repository import Qrtr
        qrtr_bus = await Qrtr.Bus.new(int(timeout * 1000))
        qrtr_node = qrtr_bus.peek_node(0) # qrtr 0
        device = await Qmi.Device.new_from_node(qrtr_node)
    else:
        raise RuntimeError(f"Unknown QMI uri {qmi_uri}")

    if not device:
        print(f"Couldn't create QMI device for {qmi_uri}")
        return

    print(f"Opening QMI: {qmi_uri}")
    await device.open(open_flags, timeout)

    print(f"Connecting to uim")
    uim = await device.allocate_client(Qmi.Service.UIM, Qmi.CID_NONE, timeout)
    print(f"UIM Get Card status")
    resp = await uim.get_card_status(None, timeout)
    card_status = resp.get_card_status()

    # parse card status
    idx = -1
    prim_gw, prim_1x, secon_gw, secon_1x, states = card_status
    for state in states:
        idx += 1
        for app in state.applications:
            print(f"UIM Card {idx} App state: {app.state.name}")
            print(f"UIM Card {idx} App type:  {app.type.name}")
            print(f"UIM Card {idx} aid as bytes:  {bytes(app.application_identifier_value).hex()}")

    print(f"Releasing the client")
    await device.release_client(uim, Qmi.DeviceReleaseClientFlags.RELEASE_CID, timeout=3)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='qmi-asyncio-example')
    parser.add_argument('-d', '--device',
        required=True,
        help="The QMI device uri. On smartphones use 'qrtr://0'. On Laptop use '/dev/cdc-wdm0'")
    args = parser.parse_args()
    # call this script with the qmi uri
    task = LOOP.create_task(main(args.device))
    LOOP.run_until_complete(task)
