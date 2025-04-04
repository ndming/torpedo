## Build instructions
The default [Release build](#release-build) is generally recommended for consuming the library, while the [Debug build](#debug-build)
is suitable for experiments and extensions.

### Build environment
It is highly recommended to use Clang for optimal compatibility and performance. While other toolchains may work, 
they have not been carefully tested and may cause issues during compilation.

There are two ways to prepare a build environment for `torpedo`:
- Using tools and dependencies already available on your system, as long as they can be detected by CMake.
- Using a Conda environment to manage dependencies, including Vulkan and build tools like CMake, Ninja, etc.

The Conda build pattern is preferred as it ensures `torpedo` is well-contained and avoids the need for administrative 
privileges when installing tools or dependencies. The repo provides `.yml` files to set up a Conda environment with
all necessary packages for each OS, and they assume no prerequisites on the host system.

#### Windows
Currently on Windows, Visual Studio version `>=17.9.7` is required for **both** build patterns. The library only needs 
the VS [BuildTools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) with the following components in *Desktop development with C++*:
- MSVC v143 - VS2022 C++ x86/64 build tools (MSVC `>=19.39`)
- Windows SDK (either 10 or 11)

###### Using a Conda environment
Set up the environment with `conda`/`mamba`:
```shell
conda env create --file env-win64.yml
conda activate torpedo
```

Additionally, the [Slang](https://github.com/shader-slang/slang/releases/tag/v2024.17) compiler version `2024.17` needs 
to be downloaded and extracted to a location whose path is specified during CMake configuration.

###### Using system packages
The following components are required:
- `CMake` version `3.25` or greater
- `Ninja`
- `Clang` version `19.1.7` or greater
- `VulkanSDK` version `1.4.304` or greater with the following components: `glslangValidator`, `slangc`, and `VMA`

#### Linux
There is no need for GCC on Linux, as the build favors Clang by default.

###### Using a Conda environment
Set up the environment with `conda`/`mamba`:
```shell
conda env create --file env-linux.yml
conda activate torpedo
```

Additionally, the [Slang](https://github.com/shader-slang/slang/releases/tag/v2024.17) compiler version `2024.17` needs 
to be downloaded and extracted to a location whose path is specified during CMake configuration.

There is a small limitation when setting up a full Conda environment for `torpedo`: the `xorg-dev` library, which
provides compatibility with X11, is not well maintained on `conda-forge`. This only causes issues when performing
surface rendering on systems without Wayland. As long as `tpd::SurfaceRenderer` is not used on such systems, the 
Conda environment works gracefully at runtimes.

###### Using system packages
The following components are required:
- `CMake` version `3.25` or greater
- `Ninja`
- `Clang` version `19.1.7` or greater
- `VulkanSDK` version `1.4.304` or greater with the following components: `glslangValidator`, `slangc`, and `VMA`

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
building `torpedo` within the Conda environment if the compiler is not installed in default search paths. 
- `-DCMAKE_INSTALL_PREFIX` (`PATH`): automatically set to `CONDA_PREFIX` if the variable is defined and the option is not 
explicitly set on the CLI. Note that `CONDA_PREFIX` is automatically defined when a `conda`/`mamba` environment activated.

</details>

Install the library:
```shell
cmake --install build
```

<details>
<summary><span style="font-weight: bold;">Notes when using a Conda environment</span></summary>

- The installation path is automatically set to `CONDA_PREFIX` unless `CMAKE_INSTALL_PREFIX` is explicitly set during 
CMake configuration.
- The `-DSLANG_COMPILER_DIR` may need to be explicitly set to the **directory** containing `slangc` to help CMake find it 
if the compiler is not installed in system's default search paths (i.e. when not using a VulkanSDK):
```shell
cmake -B build -G Ninja -DSLANG_COMPILER_DIR=path/to/dir
```
- If the system already has VulkanSDK installed but building `torpedo` from within Conda is desirable, the `VULKAN_SDK` 
environment variable must be set to `CONDA_PREFIX` (Linux) or `CONDA_PREFIX/Library` (Windows) prior to configuration.
- Additionally, if your system already installs a default compiler, it may be necessary to specify Clang for CMake:
```shell
# Replace with the full path to clang/clang++ in the conda environment
# if the system also has Clang installed
cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```
- The C/C++ compilers detected by CMake should ideally be like the following:
```
-- The C compiler identification is Clang 19.1.7
-- The CXX compiler identification is Clang 19.1.7
```

</details>

### Debug build
To configure for Debug build, define the `-DCMAKE_BUILD_TYPE` as `Debug`:
```shell
cmake -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build build
```

For debug runs, the library requests and enables the `VK_LAYER_KHRONOS_validation` layer. This was not included in the 
provided `.yml` files for the Conda build pattern and must be installed from `conda-forge`:
```shell
conda install -c conda-forge lldb=19.1.7 vulkan-validation-layers=1.4.304
```

To debug with Conda-managed dependencies, set the `VK_LAYER_PATH` environment variable to the directory containing the 
installed layers, enabling the Vulkan loader to locate them.
```shell
# Windows (PowerShell)
$env:VK_LAYER_PATH="$env:CONDA_PREFIX/Library/bin"

# Linux
export VK_LAYER_PATH=$CONDA_PREFIX/share/vulkan/explicit_layer.d
```

> [!IMPORTANT]
> The `VK_LAYER_PATH` environment variable is ignored if the library is being consumed inside a shell WITH elevated privileges, 
> see the [docs](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md) for more information.

To set this variable each time the `torpedo` environment is activated and unset it when exiting the environment, 
an activate/deactivate script can be set up to automate the process:
- On Windows (with PowerShell):
```shell
New-Item -Path "$env:CONDA_PREFIX\etc\conda\activate.d\torpedo_activate.ps1" -ItemType File
Add-Content -Path "$env:CONDA_PREFIX\etc\conda\activate.d\torpedo_activate.ps1" -Value '$env:VK_LAYER_PATH="$env:CONDA_PREFIX\Library\bin"'
New-Item -Path "$env:CONDA_PREFIX\etc\conda\deactivate.d\torpedo_deactivate.ps1" -ItemType File
Add-Content -Path "$env:CONDA_PREFIX\etc\conda\deactivate.d\torpedo_deactivate.ps1" -Value 'Remove-Item env:VK_LAYER_PATH'
```

- On Linux:
```shell
echo 'export VK_LAYER_PATH=$CONDA_PREFIX/share/vulkan/explicit_layer.d' > $CONDA_PREFIX/etc/conda/activate.d/torpedo_activate.sh
echo 'unset VK_LAYER_PATH' > $CONDA_PREFIX/etc/conda/deactivate.d/torpedo_deactivate.sh
```
