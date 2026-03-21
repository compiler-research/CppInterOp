import subprocess
import os
import sys


class CommandState:
    def __init__(self, name, command, cwd, env=None, makedirs = True):
        self.name = name
        self.command = command
        self.cwd = cwd
        self.env = env
        self.disabled = False
        self.makedirs = makedirs

    def run(self, env=None):
        if self.disabled:
            return True
        print(f"Running {self.name}: {self.command}")
        if self.makedirs:
            if not os.path.isdir(self.cwd):
                os.makedirs(self.cwd)
        if env is None:
            env = self.env
        process = subprocess.run(self.command, shell=True, cwd=self.cwd, env=env)
        if process.returncode != 0:
            print(f"Error: {self.name} failed with return code {process.returncode}")
            return False
        return True

    def disable(self):
        self.disabled = True


class CommandStateSequence:
    def __init__(self, current_env=None):
        self.command_list = []
        if current_env is None:
            current_env = os.environ.copy()
        self.current_env = current_env

    def append_command(self, cmd_name, cmd_string, cmd_dir_path):
        self.command_list.append(
            CommandState(cmd_name, cmd_string, cmd_dir_path, self.current_env.copy())
        )

    def append_clone(self, cmd_name, cmd_string, clone_dir_path):
        parent_dir_path = os.path.dirname(clone_dir_path)
        if not os.path.isdir(clone_dir_path):
            cmd_state = CommandState(cmd_name, cmd_string, parent_dir_path, self.current_env.copy(), makedirs=False)
            old_run = cmd_state.run
            def new_run(env=None):
                if not os.path.isdir(parent_dir_path):
                    os.makedirs(parent_dir_path)
                return old_run(env=env)
            cmd_state.run = new_run
            self.command_list.append(cmd_state)
        else:
            print(f"Skipping {cmd_name}: '{clone_dir_path}' already exists")


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
