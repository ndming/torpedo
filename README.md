# torpedo
Extensible rendering infrastructure using the Vulkan API.

## Usage
See [demo](demo) samples for more details.

## Build Instructions
The default [Release build](#release-build) is generally recommended for consuming the library, while the [Debug build](#debug-build)
is suitable for experiments and extensions.

### Build environment
There are two options for preparing a build environment:
- Using tools and dependencies already available on your system.
- Using a Conda environment to manage dependencies, including Vulkan and build tools like CMake, Ninja, etc.

Using Conda to manage dependencies ensures `torpedo` is well-contained and avoids the need for extra admin privileges 
when installing tools or packages. The repo provides `.yml` files to help set up a Conda environment with all necessary
dependencies for each OS, and they assume minimal prerequisites on the host system.

#### 🖥️ Build using system-wide packages
Required components for all operating systems:
- `CMake` version `3.25` or greater
- `Ninja`
- `VulkanSDK` version `1.4.304` or greater, including: `glslangValidator`, `slangc`, and `VMA`

###### Windows
Visual Studio version `>=17.12.7`, including:
- MSVC v143 - VS2022 C++ x86/64 build tools (MSVC `>=19.42`)
- Windows SDK (10/11)

To build with Clang (targeting MSVC), install and ensure LLVM version is `>=19.1.7`.

###### Linux
The library only supports building with Clang on Linux:
- `Clang` version `19.1.7` or greater
- Additional packages for GLFW, see their [docs](https://www.glfw.org/docs/latest/compile.html) for more information:
```shell
# Debian, for example:
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev
```

#### 🐍 Build using a Conda environment
Clang-build is the only option for conda-managed dependencies where all required packages and tools for each OS 
are included in their respective `.yml` files.

The only exception to the above is the [Slang](https://shader-slang.org/) compiler, for which the prebuilt
[binaries](https://github.com/shader-slang/slang/releases/tag/vulkan-sdk-1.4.304.1) corresponding to Vulkan `1.4.304` 
needs to be downloaded and extracted to a location whose path is specified during CMake configuration:
```shell
# During CMake configuration
cmake ... -DSLANG_COMPILER_DIR=path/to/slang/bin
```

###### Windows
```shell
conda env create --file env-win64.yml
conda activate torpedo
```

As Clang-build on Windows targets MSVC (unless working in Cygwin), Visual Studio is still required, for which the VS
[BuildTools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) is already sufficient.

Make sure Visual Studio version is `17.9.7` or greater, with the following components selected:
- MSVC v143 - VS2022 C++ x86/64 build tools (MSVC `>=19.39`)
- Windows SDK (10/11)

###### Linux (experimental)
```shell
conda env create --file env-linux.yml
conda activate torpedo
```

### Release build
Configure and build the project:
```shell
cmake -B build -G Ninja
cmake --build build
```

<details>
<summary><span style="font-weight: bold;">There are additional CMake options to further fine-tune the configuration</span></summary>

- `-DTORPEDO_BUILD_DEMO` (`BOOL`): build demo targets, enabled automatically for Debug build if not explicitly set on
the CLI. For other builds, the default option is `OFF` unless explicitly set otherwise on the CLI.
- `-DSLANG_COMPILER_DIR` (`PATH`): path to the directory containing the `slangc` compiler. This option is necessary when
building `torpedo` using a Conda environment if the compiler is not installed in default search paths.
- `-DCMAKE_INSTALL_PREFIX` (`PATH`): automatically set to `CONDA_PREFIX` if the variable is defined and the option is not
explicitly set on the CLI. Note that `CONDA_PREFIX` is automatically defined when a `conda`/`mamba` environment activated.

</details>

<details>
<summary><span style="font-weight: bold;">Notes when using Conda environment</span></summary>

- The installation path is automatically set to `CONDA_PREFIX` unless `CMAKE_INSTALL_PREFIX` is explicitly set during
CMake configuration.

- The `-DSLANG_COMPILER_DIR` may need to be explicitly set to the **directory** containing `slangc` to help CMake find it
if the compiler is not installed in system's default search paths (i.e. when not using a VulkanSDK):
```shell
cmake -B build -G Ninja -DSLANG_COMPILER_DIR=path/to/slangc/dir
```

- If the system already has VulkanSDK installed but building `torpedo` from within Conda is desirable, the `VULKAN_SDK`
environment variable must be set to `CONDA_PREFIX` (Linux) or `CONDA_PREFIX/Library` (Windows) prior to configuration.

- Additionally, if your system already installs a default compiler, it may be necessary to specify Clang for CMake:
```shell
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

# Or, if the system also has Clang installed
cmake -B build -G Ninja -DCMAKE_C_COMPILER=path/to/clang/in/conda/env -DCMAKE_CXX_COMPILER=path/to/clang++/in/conda/env
```

</details>

### Debug build
To configure for Debug build, define the `-DCMAKE_BUILD_TYPE` as `Debug`:
```shell
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build
```

<details>
<summary><span style="font-weight: bold;">Notes when using Conda environment</span></summary>

- For debug runs, the library requests and enables the `VK_LAYER_KHRONOS_validation` layer. This was not included in the
provided `.yml` files and must be installed from `conda-forge`:
```shell
conda install -c conda-forge lldb=19.1.7 vulkan-validation-layers=1.4.304
```

- At runtimes, set the `VK_LAYER_PATH` environment variable to the directory containing the installed layers:
```shell
# Windows (PowerShell)
$env:VK_LAYER_PATH="$env:CONDA_PREFIX/Library/bin"

# Linux
export VK_LAYER_PATH=$CONDA_PREFIX/share/vulkan/explicit_layer.d
```

- Note that the `VK_LAYER_PATH` environment variable is ignored if the library is being consumed inside a shell WITH 
elevated privileges, see the [docs](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md) 
for more information.

- To set this variable each time the `torpedo` environment is activated and unset it when exiting the environment,
an activate/deactivate script can be set up to automate the process:

###### On Windows (with PowerShell):
```shell
New-Item -Path "$env:CONDA_PREFIX\etc\conda\activate.d\torpedo_activate.ps1" -ItemType File
Add-Content -Path "$env:CONDA_PREFIX\etc\conda\activate.d\torpedo_activate.ps1" -Value '$env:VK_LAYER_PATH="$env:CONDA_PREFIX\Library\bin"'
New-Item -Path "$env:CONDA_PREFIX\etc\conda\deactivate.d\torpedo_deactivate.ps1" -ItemType File
Add-Content -Path "$env:CONDA_PREFIX\etc\conda\deactivate.d\torpedo_deactivate.ps1" -Value 'Remove-Item env:VK_LAYER_PATH'
```

###### On Linux:
```shell
echo 'export VK_LAYER_PATH=$CONDA_PREFIX/share/vulkan/explicit_layer.d' > $CONDA_PREFIX/etc/conda/activate.d/torpedo_activate.sh
echo 'unset VK_LAYER_PATH' > $CONDA_PREFIX/etc/conda/deactivate.d/torpedo_deactivate.sh
```

</details>

## External dependencies
Third-party dependencies used by `torpedo`:
- [GLFW](https://www.glfw.org/)
- [EnTT](https://github.com/skypjack/entt)
- [PLOG](https://github.com/SergiusTheBest/plog)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
