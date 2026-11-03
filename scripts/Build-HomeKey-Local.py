#!/usr/bin/env python3
"""Build committed local HomeKey branches using the recorded dependency snapshot."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess


def run(*args):
    subprocess.run(args, check=True)


def main():
    source = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=source / ".pio/homekey-local",
                        help="New directory for isolated checkouts; never overwrites an existing directory")
    parser.add_argument("--pio", default=shutil.which("pio"), help="PlatformIO executable path")
    parser.add_argument("-e", "--environment", action="append",
                        choices=["debug_RP2040", "debug_ESP32S3_HomeKey", "debug_ESP32S3_PN7160_Peripherals", "debug_ESP32S3_AppleHomeKey", "debug_ESP32S3_AppleHomeKey_IP"],
                        help="Build selected target; default: all four RP2040 / ESP32 targets")
    args = parser.parse_args()
    if not args.pio:
        parser.error("PlatformIO is not on PATH; supply --pio /path/to/pio")
    destination = args.build_dir.resolve()
    if destination.exists():
        parser.error(f"{destination} already exists; choose a new --build-dir")

    dependencies = []
    for line in (source / "dependencies.txt").read_text(encoding="utf-8-sig").splitlines():
        fields = line.split()
        if len(fields) != 4 or not fields[2].startswith("lib/"):
            continue
        revision, _, relative, _ = fields
        repository = (source / relative).resolve()
        run("git", "-C", str(repository), "cat-file", "-e", f"{revision}^{{commit}}")
        dependencies.append((revision, relative, repository))
    if not dependencies:
        parser.error("No dependency snapshot found")

    branch = subprocess.check_output(
        ["git", "-C", str(source), "branch", "--show-current"], text=True
    ).strip()
    if branch != "v1dev_HomeKey":
        parser.error("Run from the local v1dev_HomeKey application branch")
    print("Building committed sources in isolated local clones; no push.", flush=True)
    destination.parent.mkdir(parents=True, exist_ok=True)
    run("git", "clone", "--quiet", "--no-hardlinks", "--branch", branch,
        str(source), str(destination))
    for revision, relative, repository in dependencies:
        target = destination / relative
        if not target.is_symlink():
            raise RuntimeError(f"Expected library link: {target}")
        target.unlink()
        run("git", "clone", "--quiet", "--no-hardlinks", str(repository), str(target))
        run("git", "-C", str(target), "switch", "--quiet", "-C", branch, revision)
        print(f"{relative}: {revision}", flush=True)
    command = [args.pio, "run", "-d", str(destination)]
    for environment in args.environment or ["debug_RP2040", "debug_ESP32S3_HomeKey", "debug_ESP32S3_PN7160_Peripherals", "debug_ESP32S3_AppleHomeKey"]:
        command.extend(["-e", environment])
    # Inherit PLATFORMIO_CORE_DIR so callers can use an isolated toolchain cache.
    build_environment = os.environ.copy()
    # A shared /tmp/libdeps can be replaced by another application's build.
    import hashlib
    cache_id = hashlib.sha256(str(destination).encode()).hexdigest()[:12]
    build_environment["PLATFORMIO_LIBDEPS_DIR"] = f"/tmp/oam-accesscontrol-libdeps-{cache_id}"
    subprocess.run(command, check=True, env=build_environment)


if __name__ == "__main__":
    main()
