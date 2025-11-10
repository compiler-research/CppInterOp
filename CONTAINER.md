# Containers: build and run guidance

This repository includes artifacts to build CppInterOp inside containers for development and demos, with special support for WebAssembly builds via Emscripten.

Files added:

- `scripts/container_build.py` — Python wrapper around CMake with automatic EMSDK setup. Supports platforms: linux, macos, windows, emscripten. Can bootstrap EMSDK automatically or use a pre-installed version.
- `Dockerfile` — Ubuntu 24.04 image with build dependencies and pre-installed EMSDK. The `ENTRYPOINT` calls the build script. Exposes port 8080 for demos.
- `apptainer.def` — Apptainer/Singularity definition usable on HPC systems where Docker is unavailable.

Quick usage examples

1) Build locally for Emscripten with smoke test (will bootstrap EMSDK if needed):

   python3 scripts/container_build.py --platform emscripten --source . --build-dir out/build --install-dir out/install --jobs 2 --smoke-test

   Optional flags:
   - `--emsdk-version 3.1.45` to specify EMSDK version
   - `--cache-dir ~/.cache/cppinterop-build` to set tool cache location
   - `--smoke-test` to build and run the smoke test
   - `--smoke-test-browser` for Emscripten, shows URL to open test in browser

2) Build inside Docker (interactive example, with pre-installed EMSDK):

   # Build with default EMSDK version
   # Basic build (no CUDA)
   docker build -t cppinterop:dev .

   # Build with CUDA support
   docker build -t cppinterop:cuda --build-arg USE_CUDA=1 .

   # Run with CUDA GPU support
   docker run --rm -it --gpus all \
     -v "$(pwd)":/home/builder/src \
     -w /home/builder/src \
     cppinterop:cuda --platform linux --build-dir /home/builder/build --install-dir /home/builder/install --jobs 4 --detect-gpu

   # Check GPU status in container
   docker run --rm -it --gpus all cppinterop:cuda python3 /home/builder/src/scripts/container_build.py --detect-gpu

   GPU note: to expose an NVIDIA GPU to the container use `--gpus all` (or legacy `--runtime=nvidia`) when running the container. The build script only prints guidance when `--use-gpu` is passed.

3) Build with Apptainer (Singularity):

   apptainer build cppinterop.sif apptainer.def
   apptainer exec cppinterop.sif --platform linux --build-dir /tmp/build --install-dir /tmp/install

   # Check GPU support in Apptainer
   apptainer exec --nv cppinterop.sif python3 /opt/cppinterop/container_build.py --detect-gpu

GPU Requirements:
1. Host system needs:
   - NVIDIA GPU with recent drivers
   - For Docker: NVIDIA Container Toolkit installed
   - For Apptainer: NVIDIA drivers properly configured
2. Container runtime flags:
   - Docker: use `--gpus all` or legacy `--runtime=nvidia`
   - Apptainer: use `--nv` flag
3. Container image:
   - Use CUDA-enabled image: `docker build --build-arg USE_CUDA=1`
   - Or use basic image and install CUDA tools as needed

CI/CD Pipeline
-------------

Container images are automatically built and published to the GitHub Container Registry (GHCR) when:
1. Changes are pushed to `main` affecting container-related files
2. A new version tag is pushed (e.g., `v1.2.3`)
3. Manually triggered via GitHub Actions UI

Available Images:
- `ghcr.io/compiler-research/cppinterop:latest` - Latest stable build
- `ghcr.io/compiler-research/cppinterop:cuda` - CUDA-enabled build
- `ghcr.io/compiler-research/cppinterop:<version>` - Specific version (e.g., v1.2.3)

The CI pipeline:
1. Builds both regular and CUDA-enabled images
2. Runs smoke tests on the base image
3. Verifies GPU detection in CUDA image
4. Pushes images to GHCR (on main branch or tags only)
5. Updates README badge

To create a new release:
```bash
./scripts/tag-release.sh v1.2.3
```

Development Notes:
- Pull requests automatically build and test containers but don't push to registry
- Container build status is shown in the README badge
- Build logs available in GitHub Actions
