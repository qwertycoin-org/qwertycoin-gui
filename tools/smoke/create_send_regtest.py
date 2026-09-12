#!/usr/bin/env python3
"""Exercise create, mine, send and receive against an isolated QWC regtest node."""

from __future__ import annotations

import argparse
import json
import socket
import subprocess
import tempfile
import time
import urllib.error
import urllib.request
from pathlib import Path


ATOMIC_QWC = 100_000_000


def free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def post(url: str, payload: dict, timeout: float = 10.0) -> dict:
    request = urllib.request.Request(
        url,
        data=json.dumps(payload, separators=(",", ":")).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        body = json.load(response)
    if "error" in body:
        raise RuntimeError(f"RPC error {body['error'].get('code')}: {body['error'].get('message')}")
    return body


def wallet_rpc(port: int, method: str, params: dict | None = None) -> dict:
    body = post(
        f"http://127.0.0.1:{port}/json_rpc",
        {"jsonrpc": "2.0", "id": "qwc-gui-smoke", "method": method, "params": params or {}},
    )
    return body.get("result", {})


def daemon_rpc(port: int, path: str, params: dict | None = None) -> dict:
    return post(f"http://127.0.0.1:{port}/{path.lstrip('/')}", params or {})


def daemon_json_rpc(
    port: int, method: str, params: dict | None = None, timeout: float = 10.0
) -> dict:
    body = post(
        f"http://127.0.0.1:{port}/json_rpc",
        {"jsonrpc": "2.0", "id": "qwc-gui-smoke", "method": method, "params": params or {}},
        timeout=timeout,
    )
    return body.get("result", {})


def wait_for_rpc(callable_rpc, name: str, processes: list[subprocess.Popen], timeout: float = 30.0) -> None:
    deadline = time.monotonic() + timeout
    last_error: Exception | None = None
    while time.monotonic() < deadline:
        for process in processes:
            if process.poll() is not None:
                raise RuntimeError(f"{name} process exited with status {process.returncode}")
        try:
            callable_rpc()
            return
        except (OSError, RuntimeError, urllib.error.URLError) as error:
            last_error = error
            time.sleep(0.25)
    raise RuntimeError(f"timed out waiting for {name}: {last_error}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("build_dir", type=Path, help="GUI build directory containing bin/")
    parser.add_argument("--work-dir", type=Path)
    args = parser.parse_args()

    build_dir = args.build_dir.resolve()
    daemon = build_dir / "bin" / "qwertycoind"
    wallet_rpc_binary = build_dir / "bin" / "qwertycoin-wallet-rpc"
    for binary in (daemon, wallet_rpc_binary):
        if not binary.is_file():
            raise SystemExit(f"missing required binary: {binary}")

    owned_work_dir = args.work_dir is None
    work_dir = args.work_dir.resolve() if args.work_dir else Path(
        tempfile.mkdtemp(prefix="qwc-gui-create-send-")
    )
    work_dir.mkdir(parents=True, exist_ok=True)
    daemon_dir = work_dir / "daemon"
    wallet_a_dir = work_dir / "wallet-a"
    wallet_b_dir = work_dir / "wallet-b"
    for directory in (daemon_dir, wallet_a_dir, wallet_b_dir):
        directory.mkdir(parents=True, exist_ok=True)

    daemon_rpc_port = free_port()
    daemon_p2p_port = free_port()
    daemon_zmq_port = free_port()
    wallet_a_port = free_port()
    wallet_b_port = free_port()
    processes: list[subprocess.Popen] = []
    log_handles = []

    def start(command: list[str], log_name: str) -> subprocess.Popen:
        handle = (work_dir / log_name).open("wb")
        log_handles.append(handle)
        process = subprocess.Popen(command, stdout=handle, stderr=subprocess.STDOUT)
        processes.append(process)
        return process

    try:
        daemon_process = start(
            [
                str(daemon), "--regtest", "--offline", "--fixed-difficulty", "10",
                "--p2p-bind-ip", "127.0.0.1", "--p2p-bind-port", str(daemon_p2p_port),
                "--rpc-bind-ip", "127.0.0.1", "--rpc-bind-port", str(daemon_rpc_port),
                "--zmq-rpc-bind-ip", "127.0.0.1", "--zmq-rpc-bind-port", str(daemon_zmq_port),
                "--non-interactive", "--disable-dns-checkpoints", "--check-updates", "disabled",
                "--rpc-ssl", "disabled", "--data-dir", str(daemon_dir), "--log-level", "1",
            ],
            "daemon.log",
        )
        wait_for_rpc(lambda: daemon_rpc(daemon_rpc_port, "get_height"), "daemon", [daemon_process])

        def wallet_command(port: int, wallet_dir: Path) -> list[str]:
            return [
                str(wallet_rpc_binary), "--wallet-dir", str(wallet_dir),
                "--rpc-bind-ip", "127.0.0.1", "--rpc-bind-port", str(port),
                "--disable-rpc-login", "--rpc-ssl", "disabled", "--daemon-ssl", "disabled",
                "--daemon-address", f"127.0.0.1:{daemon_rpc_port}",
                "--trusted-daemon", "--allow-mismatched-daemon-version", "--log-level", "1",
            ]

        wallet_a_process = start(wallet_command(wallet_a_port, wallet_a_dir), "wallet-a.log")
        wallet_b_process = start(wallet_command(wallet_b_port, wallet_b_dir), "wallet-b.log")
        wait_for_rpc(lambda: wallet_rpc(wallet_a_port, "get_version"), "wallet A", [wallet_a_process])
        wait_for_rpc(lambda: wallet_rpc(wallet_b_port, "get_version"), "wallet B", [wallet_b_process])

        password = "isolated-qwc-gui-smoke"
        wallet_rpc(wallet_a_port, "create_wallet", {"filename": "wallet-a", "password": password, "language": "English"})
        wallet_rpc(wallet_b_port, "create_wallet", {"filename": "wallet-b", "password": password, "language": "English"})
        address_a = wallet_rpc(wallet_a_port, "get_address")["address"]
        address_b = wallet_rpc(wallet_b_port, "get_address")["address"]
        if not address_a.startswith("QWC") or not address_b.startswith("QWC") or address_a == address_b:
            raise RuntimeError("wallet RPC did not create two distinct primary QWC addresses")

        daemon_json_rpc(
            daemon_rpc_port,
            "generateblocks",
            {"wallet_address": address_a, "amount_of_blocks": 80, "reserve_size": 20},
            timeout=180,
        )
        wallet_rpc(wallet_a_port, "refresh")
        balance_a = wallet_rpc(wallet_a_port, "get_balance")
        if int(balance_a.get("unlocked_balance", 0)) < ATOMIC_QWC:
            raise RuntimeError("wallet A did not receive an unlocked regtest mining output")

        transfer = wallet_rpc(
            wallet_a_port,
            "transfer",
            {
                "destinations": [{"amount": ATOMIC_QWC, "address": address_b}],
                "priority": 0,
                "get_tx_key": True,
            },
        )
        tx_hash = transfer.get("tx_hash", "")
        if len(tx_hash) != 64:
            raise RuntimeError("wallet A did not return a canonical transaction hash")

        daemon_json_rpc(
            daemon_rpc_port,
            "generateblocks",
            {"wallet_address": address_a, "amount_of_blocks": 1, "reserve_size": 20},
            timeout=60,
        )
        wallet_rpc(wallet_b_port, "refresh")
        balance_b = wallet_rpc(wallet_b_port, "get_balance")
        if int(balance_b.get("balance", 0)) < ATOMIC_QWC:
            raise RuntimeError("wallet B did not receive the confirmed QWC transfer")

        print("create-send regtest smoke passed")
        print("created_wallets=2")
        print("confirmed_transfer_atomic=100000000")
        print("recipient_balance_verified=true")
        print(f"work_dir={work_dir}")
        return 0
    finally:
        for process in reversed(processes):
            if process.poll() is None:
                process.terminate()
        deadline = time.monotonic() + 10
        for process in reversed(processes):
            if process.poll() is None:
                try:
                    process.wait(max(0.1, deadline - time.monotonic()))
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
        for handle in log_handles:
            handle.close()
        if owned_work_dir:
            print("temporary_wallet_files_retained_for_review=true")


if __name__ == "__main__":
    raise SystemExit(main())
