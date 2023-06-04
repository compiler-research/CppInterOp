# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'CppInterOp'
copyright = '2023, Vassil Vassilev'
author = 'Vassil Vassilev'
release = 'Dev'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = []

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'alabaster'
html_static_path = ['_static']

INTEROP_ROOT = '..'

# html_extra_path = [INTEROP_ROOT + '/build/docs/doxygen/html']

import subprocess
command = 'mkdir {0}/build; cd {0}/build; cmake ../ -DClang_DIR=/usr/lib/llvm-14/lib/cmake/clang\
         -DLLVM_DIR=/usr/lib/llvm-14 -DINTEROP_ENABLE_DOXYGEN=ON\
         -DINTEROP_INCLUDE_DOCS=ON'.format(INTEROP_ROOT)
subprocess.call(command, shell=True)
subprocess.call('doxygen {0}/build/docs/doxygen.cfg'.format(INTEROP_ROOT), shell=True)