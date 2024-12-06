highlight_language = "C++"

todo_include_todos = True

mathjax_path = "https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"

# Add latex physics package
mathjax3_config = {
    "loader": {"load": ["[tex]/physics"]},
    "tex": {"packages": {"[+]": ["physics"]}},
}

"""documentation for CppInterOp"""
import datetime
import json
import os
import subprocess
import sys
from pathlib import Path

from sphinx.application import Sphinx

os.environ.update(IN_SPHINX="1")

CONF_PY = Path(__file__)
HERE = CONF_PY.parent
ROOT = HERE.parent
RTD = json.loads(os.environ.get("READTHEDOCS", "False").lower())

# tasks that won't have been run prior to building the docs on RTD
RTD_PRE_TASKS = ["build", "docs:typedoc:mystify", "docs:app:pack"]

# metadata
author = 'Vassil Vassilev'
project = 'CppInterOp'
copyright = '2023, Vassil Vassilev'
release = 'Dev'

# sphinx config
extensions = []

autosectionlabel_prefix_document = True
myst_heading_anchors = 3
suppress_warnings = ["autosectionlabel.*"]

# files
templates_path = ["_templates"]
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

html_css_files = [
    "doxygen.css",
]

# theme
html_theme = 'alabaster'
html_theme_options = {
    "github_user": "compiler-research",
    "github_repo": "CppInterOp",
    "github_banner": True,
    "fixed_sidebar": True,
}

html_context = {
    "github_user": "compiler-research",
    "github_repo": "CppInterOp",
    "github_version": "main",
    "doc_path": "docs",
}


def do_tasks(label, tasks):
    """Run some doit tasks before/after the build"""
    task_rcs = []

    for task in tasks:
        print(f"[CppInterOp-docs] running {label} {task}", flush=True)
        rc = subprocess.call(["doit", "-n4", task], cwd=str(ROOT))

        if rc != 0:
            rc = subprocess.call(["doit", task], cwd=str(ROOT))

        print(f"[CppInterOp-docs] ... ran {label} {task}: returned {rc}", flush=True)
        task_rcs += [rc]

    if max(task_rcs) > 0:
        raise Exception("[CppInterOp-docs] ... FAIL, see log above")

    print(f"[CppInterOp-docs] ... {label.upper()} OK", flush=True)


def before_rtd_build(app: Sphinx, error):
    """ensure doit docs:sphinx precursors have been met on RTD"""
    print("[CppInterOp-docs] Staging files changed by RTD...", flush=True)
    subprocess.call(["git", "add", "."], cwd=str(ROOT))
    do_tasks("post", RTD_PRE_TASKS)


def after_build(app: Sphinx, error):
    """sphinx-jsonschema makes duplicate ids. clean them"""
    os.environ.update(JLITE_DOCS_OUT=app.builder.outdir) 


def setup(app):
    app.connect("build-finished", after_build)
    if RTD:
        app.connect("config-inited", before_rtd_build)
