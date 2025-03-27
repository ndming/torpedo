## Build instructions
The default [Release build](#release-build) is generally recommended for consuming the library, while the [Debug build](#debug-build)
is suitable for experiments and extensions.

### Prerequisites
There are no prerequisite dependencies for building the library on Linux since all dependencies, including Vulkan, 
are going to be managed via an isolated `conda` environment.

On Windows, MSVC version `>=19.39` must be installed via Visual Studio (version `>=17.9.7`). The library only needs the VS 
[BuildTools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) with the following components 
in the *Desktop development C++* workload:
- MSVC v143 - VS2022 C++ x86/64 build tools
- Windows SDK (either 10 or 11)

Set up the environment with `conda`/`mamba`:
```shell
conda env create --file environment.yml
conda activate torpedo
```

### Release build
Configure the build to use Clang and Ninja (clang-tools are installed in the `conda` environment):
```shell
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja
```

<details>
<summary><span style="font-weight: bold;">There are additional CMake options to further fine-tune the configuration</span></summary>

- `-DCMAKE_BUILD_DEMO` (`BOOL`): build demo targets, enabled automatically for Debug build only if not explicitly set on 
the CLI. For other builds, the option is default to `OFF`, unless explicitly set otherwise on the CLI.
- `-DCMAKE_INSTALL_PREFIX` (`PATH`): automatically set to `CONDA_PREFIX` if such a variable is defined and the option is
not explicitly set on the CLI. Note that `CONDA_PREFIX` is also defined if a `mamba` environment is activated.

</details>

Build and install the library into the `conda` environment:
```shell
cmake --build build
cmake --install build
```

Downstream applications can now consume the library directly as long as they use CMake in the `conda` environment to 
configure their projects.

### Debug build
Configure and build the library in `Debug` mode:
```shell
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja
cmake --build build
```

During debug runs, the library requests and enables the `VK_LAYER_KHRONOS_validation` layer, which can be installed via `conda-forge`:
```shell
conda install -c conda-forge lldb=19.1.7 vulkan-validation-layers=1.4.304
```

Since we are not using the LunarG SDK, the `VK_LAYER_PATH` environment variable must be set to the directory containing 
the layers, allowing the Vulkan loader to locate them.

> [!IMPORTANT]
> The `VK_LAYER_PATH` environment variable is ignored if the downstream application is running WITH elevated privileges, 
> see the [docs](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md) for more information.

```shell
# Windows (PowerShell)
$env:VK_LAYER_PATH="$env:CONDA_PREFIX/Library/bin"

# Linux
export VK_LAYER_PATH=$CONDA_PREFIX/Library/bin
```

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
echo 'export VK_LAYER_PATH="$CONDA_PREFIX/Library/bin"' > $CONDA_PREFIX/etc/conda/activate.d/torpedo_activate.sh
echo 'unset VK_LAYER_PATH' > $CONDA_PREFIX/etc/conda/deactivate.d/torpedo_deactivate.sh
```
