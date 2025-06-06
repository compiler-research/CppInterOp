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

html_theme_options = {
    "github_user": "compiler-research",
    "github_repo": "CppInterOp",
    "github_banner": True,
    "fixed_sidebar": True,
}

highlight_language = "C++"

todo_include_todos = True

mathjax_path = "https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"

# Add latex physics package
mathjax3_config = {
    "loader": {"load": ["[tex]/physics"]},
    "tex": {"packages": {"[+]": ["physics"]}},
}

import os
CPPINTEROP_ROOT = os.path.abspath('..')
html_extra_path = [CPPINTEROP_ROOT + '/build/docs/']

import subprocess
command = 'mkdir {0}/build; cd {0}/build; cmake ../ -DClang_DIR=/usr/lib/llvm-16/build/lib/cmake/clang\
         -DLLVM_DIR=/usr/lib/llvm-16/build/lib/cmake/llvm -DCPPINTEROP_ENABLE_DOXYGEN=ON\
         -DCPPINTEROP_INCLUDE_DOCS=ON'.format(CPPINTEROP_ROOT)
command_emscripten = 'git clone https://github.com/emscripten-core/emsdk.git {0}/emsdk;\
                {0}/emsdk/emsdk install  3.1.73;\
                {0}/emsdk/emsdk activate 3.1.73;\
                cd {0}/emsdk/;\
                export PATH="{0}/emsdk/upstream/emscripten:$PATH";\
                export SYSROOT_PATH={0}/emsdk/upstream/emscripten/cache/sysroot;\
                git clone --depth=1 --branch release/20.x https://github.com/llvm/llvm-project.git {0}/llvm-project;\
                cd {0}/llvm-project;\
                git apply -v {0}/patches/llvm/emscripten-clang20-*.patch\
                mkdir {0}/llvm-project/native_build;\
                cd {0}/llvm-project/native_build;\
                cmake -DLLVM_ENABLE_PROJECTS=clang -DLLVM_TARGETS_TO_BUILD=host -DCMAKE_BUILD_TYPE=Release {0}/llvm-project/llvm;\
                cmake --build . --target llvm-tblgen clang-tblgen --parallel $(nproc --all);\
                export NATIVE_DIR={0}/llvm-project/native_build/bin/;\
                mkdir {0}/llvm-project/build;\
                cd {0}/llvm-project/build;\
                emcmake cmake -DCMAKE_BUILD_TYPE=Release \
                        -DLLVM_HOST_TRIPLE=wasm32-unknown-emscripten \
                        -DLLVM_ENABLE_ASSERTIONS=ON                        \
                        -DLLVM_TARGETS_TO_BUILD="WebAssembly" \
                        -DLLVM_ENABLE_LIBEDIT=OFF \
                        -DLLVM_ENABLE_PROJECTS="clang;lld" \
                        -DLLVM_ENABLE_ZSTD=OFF \
                        -DLLVM_ENABLE_LIBXML2=OFF \
                        -DCLANG_ENABLE_STATIC_ANALYZER=OFF \
                        -DCLANG_ENABLE_ARCMT=OFF \
                        -DCLANG_ENABLE_BOOTSTRAP=OFF \
                        -DCMAKE_CXX_FLAGS="-Dwait4=__syscall_wait4" \
                        -DLLVM_INCLUDE_BENCHMARKS=OFF                   \
                        -DLLVM_INCLUDE_EXAMPLES=OFF                     \
                        -DLLVM_INCLUDE_TESTS=OFF                        \
                        -DLLVM_ENABLE_THREADS=OFF                       \
                        -DLLVM_BUILD_TOOLS=OFF                          \
                        -DLLVM_ENABLE_LIBPFM=OFF                        \
                        -DCLANG_BUILD_TOOLS=OFF                         \
                        -DLLVM_NATIVE_TOOL_DIR=$NATIVE_DIR              \
                        {0}/llvm-project/llvm;\
                emmake make libclang -j $(nproc --all);\
                emmake make clangInterpreter clangStaticAnalyzerCore -j $(nproc --all);\
                emmake make lldWasm -j $(nproc --all);\
                export LLVM_BUILD_DIR={0}/llvm-project/build;\
                mkdir {0}/build_native;\
                cd {0};\
                micromamba create -f environment-wasm.yml --platform=emscripten-wasm32;\
                micromamba activate CppInterOp-wasm;\
                export PREFIX=$CONDA_PREFIX;\
                export CMAKE_PREFIX_PATH=$PREFIX;\
                export CMAKE_SYSTEM_PREFIX_PATH=$PREFIX;\
                cd {0}/build_native;\
                emcmake cmake -DCMAKE_BUILD_TYPE=Release    \
                -DLLVM_DIR=$LLVM_BUILD_DIR/lib/cmake/llvm      \
                -DLLD_DIR=$LLVM_BUILD_DIR/lib/cmake/lld     \
                -DClang_DIR=$LLVM_BUILD_DIR/lib/cmake/clang     \
                -DBUILD_SHARED_LIBS=ON                      \
                -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ON            \
                -DCMAKE_INSTALL_PREFIX=$PREFIX         \
                -DSYSROOT_PATH=$SYSROOT_PATH                                   \
                {0};\
                emake make -j $(nproc --all) install;\
                cd {0};\
                git clone --depth=1 https://github.com/compiler-research/xeus-cpp.git {0}/xeus-cpp;\
                mkdir {0}/xeus-cpp/build;\
                cd {0}/xeus-cpp/build;\
                emcmake cmake \
                        -DCMAKE_BUILD_TYPE=Release                                     \
                        -DCMAKE_PREFIX_PATH=$PREFIX                                    \
                        -DCMAKE_INSTALL_PREFIX=$PREFIX                                 \
                        -DXEUS_CPP_EMSCRIPTEN_WASM_BUILD=ON                            \
                        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ON                         \
                        -DXEUS_CPP_RESOURCE_DIR=$LLVM_BUILD_DIR/lib/clang/$LLVM_VERSION \
                        -DSYSROOT_PATH=$SYSROOT_PATH                                   \
                {0}/xeus-cpp/;\
                emmake make -j $(nproc --all) install;\
                cd {0};\
                micromamba create -n xeus-lite-host jupyterlite-core -c conda-forge;\
                micromamba activate xeus-lite-host;\
                python -m pip install jupyterlite-xeus jupyter_server;\
                jupyter lite build --XeusAddon.prefix=$PREFIX --contents xeus-cpp/notebooks/xeus-cpp-lite-demo.ipynb --contents notebooks/smallpt.ipynb --contents notebooks/images/marie.png --contents notebooks/audio/audio.wav --output-dir  $READTHEDOCS_OUTPUT/html/xeus-cpp; '.format(CPPINTEROP_ROOT)
subprocess.call(command, shell=True)
subprocess.call('doxygen {0}/build/docs/doxygen.cfg'.format(CPPINTEROP_ROOT), shell=True)
subprocess.call('doxygen {0}/build/docs/doxygen.cfg'.format(CPPINTEROP_ROOT), shell=True)
subprocess.call(command_emscripten.format(CPPINTEROP_ROOT), shell=True)
