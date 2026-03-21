import argparse
import os
import sys
import subprocess
import platform
import yaml
import re


# Helper functions

def get_user_permission(message, exit=False):
    while True:
        print()
        response = input(message + "(y/n):")
        print()
        if response.lower() == "y":
            return True
        elif response.lower() == "n":
            if exit:
                sys.exit()
            return False

def replace_var(config_variables, match):
    key = match.group(1)
    if key not in config_variables:
        raise RuntimeError(f"Unknown variable: {key}")
    return str(config_variables[key])

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

config_variables["CppInterOp"] = os.path.abspath(args.cppinterop).replace('\\', '/')
config_variables["llvm-project"] = os.path.abspath(args.llvm_project).replace('\\', '/')
config_variables["cppyy-backend"] = os.path.abspath(args.cppyy_backend).replace('\\', '/')
config_variables["cling"] = os.path.abspath(args.cling).replace('\\', '/') if args.cling else ""

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
config_text = re.sub(r"@@([A-Za-z0-9_-]+)@@", lambda match: replace_var(config_variables, match), config_text)
config_data = yaml.safe_load(config_text)

# Load pipeline.yaml

with open("pipeline.yml") as f:
    pipeline = yaml.safe_load(f)

# Create build sequence

command_list = []
current_env = os.environ.copy()


for step in pipeline["build_sequence"]:
    step_tags = set(step.get("tags", []))
    if not step_tags.issubset(active_tags):
        continue
    name = step["step"]
    step_def = config_data[name]
    step_type = step_def["type"]

    if step_type == "clone":
        clone_dir_path = os.path.abspath(step_def["dir"])
        cmd_string = step_def["content"]
        if not os.path.isdir(clone_dir_path):
            parent_dir_path = os.path.dirname(clone_dir_path)
            command_list.append({
                "cmd_name": name,
                "cmd_string": cmd_string,
                "cmd_dir_path": parent_dir_path,
                "cmd_env": current_env.copy()
            })
        else:
            print(f"Skipping {name}: '{clone_dir_path}' already exists")

    elif step_type == "command":
        cmd_dir_path = os.path.abspath(step_def["cwd"])
        cmd_string = step_def["content"]
        command_list.append({
            "cmd_name": name,
            "cmd_string": cmd_string,
            "cmd_dir_path": cmd_dir_path,
            "cmd_env": current_env.copy()
        })

    elif step_type == "env":
        env_name = step_def["name"]
        content = step_def["content"]
        if isinstance(content, str):
            current_env[env_name] = content
        elif isinstance(content, list):
            current_env[env_name] = (os.pathsep).join(content)
        else:
            raise AssertionError("Unsupported type for env content")

print("\n--- Build Plan ---")
print(f"[OS: {platform.system()}] | [Backend: {args.backend}] | [OOP JIT: {args.oop}]")

i = 0
while i < len(command_list):
    cmd_state = command_list[i]
    print(f"Step {i+1}: {cmd_state["cmd_name"]}")
    print(cmd_state["cmd_string"])
    permission = get_user_permission("Keep command in plan? ") if not args.grant_permission else True
    if not permission:
        command_list.pop(i)
    else:
        i+=1
    print()

if not args.grant_permission:
    get_user_permission("Start build? ", exit=True)

# Run pipeline

print("\n--- Starting Build Pipeline ---")

for cmd_state in command_list:
    print(f"Running {cmd_state["cmd_name"]}: {cmd_state["cmd_string"]}")
    if not os.path.isdir(cmd_dir_path):
        os.makedirs(cmd_dir_path)
    process = subprocess.run(cmd_state["cmd_string"], shell=True, cwd=cmd_state["cmd_dir_path"], env=cmd_state["cmd_env"])
    if process.returncode != 0:
        print(f"Error: {cmd_state["cmd_name"]} failed with return code {process.returncode}")
        print("Build pipeline aborted due to an error.", file=sys.stderr)
        sys.exit(1)


print("\n--- Build Complete ---")
print("\nYou may copy the following environment variables\n")


valid_env_names = [value["name"] for value in config_data.values() if value["type"] == "env"]
for key, value in current_env.items():
    if key in valid_env_names:
        if platform.system() == "Windows":
            print(f'$env:{key}="{value}"')
        else:
            print(f'export {key}="{value}"')
