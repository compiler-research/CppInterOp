import argparse
import os
import sys
import subprocess
import platform
import yaml
import re
import utils


# CLI Arguments

parser = argparse.ArgumentParser(description="Build Script for CppInterOp")

parser.add_argument("--backend", choices=["clang-repl", "cling"], required=True, help="Compiler backend to use")

parser.add_argument("--oop", action="store_true", default=False, help="Use out-of-process JIT (only for Linux x86 & MacOS arm64)")

parser.add_argument("--jobs", type=int, default=os.cpu_count(), help="Number of parallel jobs (default: number of cpus)")

parser.add_argument("--cppinterop", required=True, help="Path to CppInterOp directory")
parser.add_argument("--llvm-project", required=True, help="Path to llvm-project directory")
parser.add_argument("--cppyy-backend", required=True, help="Path to cppyy-backend directory")
parser.add_argument("--cling", help="Path to cling directory")

parser.add_argument("--grant-permission", action="store_true", default=False, help="Executes without asking for permission")

args = parser.parse_args()

# Detect OS

if not (platform.system().lower() in ["linux", "darwin", "windows"]):
    raise RuntimeError("Unsupported OS")

# Pipeline Tags

active_tags = set()

active_tags.add(platform.system().lower())

if platform.system().lower() in ["linux", "darwin"]:
    active_tags.add("unix")

active_tags.add(args.backend)

if args.oop:
    active_tags.add("oop")
else:
    active_tags.add("no-oop")

# Config Variables

config_variables = dict()

config_variables["jobs"] = args.jobs

config_variables["CppInterOp"] = os.path.abspath(args.cppinterop)
config_variables["llvm-project"] = os.path.abspath(args.llvm_project)
config_variables["cppyy-backend"] = os.path.abspath(args.cppyy_backend)
config_variables["cling"] = os.path.abspath(args.cling) if args.cling else ""

config_variables["DIR_SEP"] = os.sep
config_variables["PATH_SEP"] = os.pathsep

if platform.system() == "Darwin":
    sdkroot = subprocess.check_output(
        ["xcrun", "--show-sdk-path"], text=True
    ).strip()
    config_variables["sdkroot"] = sdkroot
else:
    config_variables["sdkroot"] = ""

# Load config.yaml

with open("config.yml") as f:
    config_text = f.read()
config_text = re.sub(r"@@([A-Za-z0-9_-]+)@@", lambda match: utils.replace_var(config_variables, match), config_text)
config_data = yaml.safe_load(config_text)

# Load pipeline.yaml

with open("pipeline.yml") as f:
    pipeline = yaml.safe_load(f)

# Create build sequence

build_sequence = utils.CommandStateSequence()

for step in pipeline["build_sequence"]:
    step_tags = set(step.get("tags", []))
    if not step_tags.issubset(active_tags):
        continue
    name = step["step"]
    step_def = config_data[name]
    step_type = step_def["type"]

    if step_type == "clone":
        directory = step_def["dir"]
        command = step_def["content"]
        build_sequence.append_clone(
            name, 
            command, 
            directory
        )

    elif step_type == "command":
        command = step_def["content"]
        directory = step_def["cwd"]
        build_sequence.append_command(name, command, directory)

    elif step_type == "env":
        env_name = step_def["name"]
        value = step_def["content"]
        build_sequence.current_env[env_name] = value

print("\n--- Build Plan ---")
print(f"[OS: {platform.system()}] | [Backend: {args.backend}] | [OOP JIT: {args.oop}]")

for i, command_state in enumerate(build_sequence.command_list, 1):
    print(f"Step {i}: {command_state.name}")
    print(command_state.command)
    permission = utils.get_user_permission("Keep command in plan? ") if not args.grant_permission else True
    if not permission:
        command_state.disable()
    print()

if not args.grant_permission:
    utils.get_user_permission("Start build? ", exit=True)

# Run pipeline

print("\n--- Starting Build Pipeline ---")

for command_state in build_sequence.command_list:
    success = command_state.run()
    if not success:
        print("Build pipeline aborted due to an error.")
        sys.exit()


print("\n--- Build Complete ---")
print("\nYou may copy the following environment variables\n")

for key, value in build_sequence.current_env.items():
    if key in [
        "LLVM_DIR",
        "CB_PYTHON_DIR",
        "CPPINTEROP_DIR",
        "CLING_DIR",
        "CLING_BUILD_DIR",
        "CPLUS_INCLUDE_PATH",
        "SDKROOT"
    ]:
        if platform.system() == "Windows":
            print(f'$env:{key}="{value}"')
        else:
            print(f'export {key}="{value}"')
