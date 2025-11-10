#!/usr/bin/env python3
"""
Minimal container-friendly build script for CppInterOp.

This script provides a small, cross-platform wrapper around CMake to build
the project from inside a container (or on the host). It intentionally keeps
options small so the Dockerfile and Apptainer definition can call it with
predictable arguments.

This is an initial, minimal implementation meant to be expanded later.
"""

from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path


def run(cmd: str, dry_run: bool = False, env: dict = None, capture_output: bool = False) -> subprocess.CompletedProcess:
    print(f"$ {cmd}")
    if dry_run:
        return subprocess.CompletedProcess([], 0)
    return subprocess.run(cmd, shell=True, check=True, env=env, 
                        capture_output=capture_output, text=capture_output)


def detect_gpu() -> dict:
    """Detect NVIDIA GPU details if available."""
    result = {
        "has_gpu": False,
        "driver_version": None,
        "cuda_version": None,
        "gpu_names": [],
        "in_container": os.path.exists("/.dockerenv"),
    }

    try:
        # Check for nvidia-smi
        nvidia_smi = run("nvidia-smi --query-gpu=gpu_name,driver_version --format=csv,noheader", 
                        capture_output=True)
        if nvidia_smi.returncode == 0:
            result["has_gpu"] = True
            for line in nvidia_smi.stdout.strip().split("\n"):
                name, driver = line.strip().split(", ")
                result["gpu_names"].append(name)
                result["driver_version"] = driver  # Last one if multiple GPUs

            # Try to get CUDA version
            nvcc = run("nvcc --version", capture_output=True)
            if nvcc.returncode == 0:
                for line in nvcc.stdout.splitlines():
                    if "release" in line.lower():
                        result["cuda_version"] = line.split("release")[-1].strip().split(",")[0]
                        break
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass

    return result


def setup_emsdk(cache_dir: Path, version: str, dry_run: bool = False) -> Path:
    """Download and install EMSDK if not already present in cache_dir."""
    emsdk_dir = cache_dir / "emsdk"
    if emsdk_dir.exists():
        print(f"Using cached EMSDK at {emsdk_dir}")
        return emsdk_dir

    # Create temp dir for initial clone, then move to cache
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        print(f"Downloading EMSDK to {tmp_path}...")
        if not dry_run:
            subprocess.run(
                ["git", "clone", "https://github.com/emscripten-core/emsdk.git", str(tmp_path)],
                check=True
            )
            subprocess.run(["./emsdk", "install", version], cwd=tmp_path, check=True)
            subprocess.run(["./emsdk", "activate", version], cwd=tmp_path, check=True)

            # Move to cache location
            cache_dir.mkdir(parents=True, exist_ok=True)
            shutil.move(str(tmp_path), str(emsdk_dir))

    if dry_run:
        print(f"[DRY RUN] Would install EMSDK {version} to {emsdk_dir}")
        return Path("/dry-run/emsdk")

    return emsdk_dir


def main() -> int:
    parser = argparse.ArgumentParser(description="Build CppInterOp from a container or host")
    
    # Special case: if only --detect-gpu is passed, skip build setup
    if len(sys.argv) == 2 and sys.argv[1] == '--detect-gpu':
        print("\nChecking GPU status...")
        gpu_info = detect_gpu()
        
        if gpu_info["has_gpu"]:
            print(f"✓ NVIDIA GPU detected:")
            for gpu in gpu_info["gpu_names"]:
                print(f"  - {gpu}")
            print(f"  Driver version: {gpu_info['driver_version']}")
            if gpu_info["cuda_version"]:
                print(f"  CUDA version: {gpu_info['cuda_version']}")
            
            if gpu_info["in_container"]:
                print("\nℹ️  Running inside container - ensure proper GPU access:")
                print("  - Docker: use '--gpus all' or '--runtime=nvidia'")
                print("  - Apptainer/Singularity: use '--nv'")
        else:
            print("✗ No NVIDIA GPU detected or drivers not accessible.")
            if gpu_info["in_container"]:
                print("\nIf you have an NVIDIA GPU, ensure:")
                print("1. NVIDIA Container Toolkit is installed on the host")
                print("2. Container is run with GPU access flags")
                print("3. Using a CUDA-enabled base image")
        return 0
    parser.add_argument("--platform", choices=["linux", "macos", "windows", "emscripten"], default="linux",
                        help="Target platform for the build (affects some cmake flags)")
    parser.add_argument("--source", default=".", help="Path to the repository root (default: current directory)")
    parser.add_argument("--build-dir", default="build", help="Path for out-of-source build directory")
    parser.add_argument("--install-dir", default="install", help="Path to install prefix")
    parser.add_argument("--jobs", type=int, default=4, help="Parallel build jobs")
    parser.add_argument("--dry-run", action="store_true", help="Print commands but don't execute")
    parser.add_argument("--use-gpu", action="store_true", help="Note: request GPU support in container runtime (no-op here)")
    parser.add_argument("--cmake-extra", default="", help="Extra flags to pass to CMake (quoted)")
    parser.add_argument("--emsdk-version", default="3.1.45",
                      help="EMSDK version to install when platform=emscripten (default: 3.1.45)")
    parser.add_argument("--cache-dir", default=os.path.expanduser("~/.cache/cppinterop-build"),
                      help="Cache directory for downloaded tools like EMSDK")
    parser.add_argument("--smoke-test", action="store_true",
                      help="Build and run the smoke test after main build")
    parser.add_argument("--smoke-test-browser", action="store_true",
                      help="For Emscripten builds, print URL to open the smoke test in a browser")
    parser.add_argument("--detect-gpu", action="store_true",
                      help="Detect and print GPU details (NVIDIA driver version, CUDA version)")

    args = parser.parse_args()

    src = Path(args.source).resolve()
    build = Path(args.build_dir).resolve()
    install = Path(args.install_dir).resolve()

    if not src.exists():
        print(f"Source directory does not exist: {src}")
        return 2

    build.mkdir(parents=True, exist_ok=True)
    install.mkdir(parents=True, exist_ok=True)

    # Basic cmake configuration — small and conservative so it works in containers
    generator = "Unix Makefiles"
    extra = args.cmake_extra.strip()

    emsdk_env = dict(os.environ)

    if args.platform == "emscripten":
        # Set up EMSDK if needed, or use system install if available
        try:
            subprocess.run(["emcc", "--version"], check=True, capture_output=True)
            print("Using system Emscripten install")
        except (subprocess.CalledProcessError, FileNotFoundError):
            print("System Emscripten not found, bootstrapping EMSDK...")
            emsdk_root = setup_emsdk(Path(args.cache_dir), args.emsdk_version, args.dry_run)
            if not args.dry_run:
                subprocess.run([str(emsdk_root / "emsdk"), "activate", args.emsdk_version], check=True)
                emsdk_env.update(
                    dict(line.split('=', 1) for line in
                         subprocess.check_output([str(emsdk_root / "emsdk"), "activate", args.emsdk_version, "--embedded"]).decode().splitlines()
                         if '=' in line)
                )

        generator = "Ninja"
        cmake_cmd = (
            f"emcmake cmake -S {shlex.quote(str(src))} -B {shlex.quote(str(build))} "
            f"-G {shlex.quote(generator)} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX={shlex.quote(str(install))} {extra}"
        )
    elif args.platform == "windows":
        # In a container-based linux build we cannot produce native Windows binaries
        # without a cross-compiler or wine/msvc toolchain. For now, this acts as a placeholder.
        cmake_cmd = (
            f"cmake -S {shlex.quote(str(src))} -B {shlex.quote(str(build))} "
            f"-G {shlex.quote(generator)} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX={shlex.quote(str(install))} {extra}"
        )
    else:
        cmake_cmd = (
            f"cmake -S {shlex.quote(str(src))} -B {shlex.quote(str(build))} "
            f"-G {shlex.quote(generator)} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX={shlex.quote(str(install))} {extra}"
        )

    build_cmd = f"cmake --build {shlex.quote(str(build))} -- -j {args.jobs}"
    install_cmd = f"cmake --install {shlex.quote(str(build))} --prefix {shlex.quote(str(install))}"

    try:
        run(cmake_cmd, dry_run=args.dry_run, env=emsdk_env)
        run(build_cmd, dry_run=args.dry_run, env=emsdk_env)
        run(install_cmd, dry_run=args.dry_run, env=emsdk_env)
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {e}")
        return 1

    if args.use_gpu or args.detect_gpu:
        print("\nChecking GPU status...")
        gpu_info = detect_gpu()
        
        if gpu_info["has_gpu"]:
            print(f"✓ NVIDIA GPU detected:")
            for gpu in gpu_info["gpu_names"]:
                print(f"  - {gpu}")
            print(f"  Driver version: {gpu_info['driver_version']}")
            if gpu_info["cuda_version"]:
                print(f"  CUDA version: {gpu_info['cuda_version']}")
            
            if gpu_info["in_container"]:
                print("\nℹ️  Running inside container - ensure proper GPU access:")
                print("  - Docker: use '--gpus all' or '--runtime=nvidia'")
                print("  - Apptainer/Singularity: use '--nv'")
        else:
            print("✗ No NVIDIA GPU detected or drivers not accessible.")
            if gpu_info["in_container"]:
                print("\nIf you have an NVIDIA GPU, ensure:")
                print("1. NVIDIA Container Toolkit is installed on the host")
                print("2. Container is run with GPU access flags")
                print("3. Using a CUDA-enabled base image")

    print("")
    print("Build finished. Install tree is at:", install)

    # Run smoke test if requested
    if args.smoke_test:
        print("\nRunning smoke test...")
        smoke_dir = src / "tests" / "smoke"
        smoke_build = build / "smoke"
        smoke_build.mkdir(parents=True, exist_ok=True)

        # Configure smoke test
        smoke_cmake = (
            f"cmake -S {shlex.quote(str(smoke_dir))} -B {shlex.quote(str(smoke_build))} "
            f"-G {shlex.quote(generator)} -DCMAKE_BUILD_TYPE=Release "
            f"-DCMAKE_PREFIX_PATH={shlex.quote(str(install))} {extra}"
        )
        if args.platform == "emscripten":
            smoke_cmake = f"emcmake {smoke_cmake}"

        try:
            run(smoke_cmake, dry_run=args.dry_run, env=emsdk_env)
            run(f"cmake --build {shlex.quote(str(smoke_build))} -- -j {args.jobs}", 
                dry_run=args.dry_run, env=emsdk_env)

            if args.platform == "emscripten":
                if args.smoke_test_browser:
                    print("\nSmoke test built. To run, start a web server and open:")
                    print(f"http://localhost:8080/{smoke_build}/smoke_test.html")
                    print("\nQuick test server: python3 -m http.server 8080")
            else:
                # Run native binary
                run(f"{shlex.quote(str(smoke_build))}/smoke_test", 
                    dry_run=args.dry_run, env=emsdk_env)
                print("Smoke test passed!")
        except subprocess.CalledProcessError as e:
            print(f"Smoke test failed: {e}")
            return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
