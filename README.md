## Build instructions
Below are the steps to build `torpedo` for use in downstream applications or for debugging during development.

### Prerequisites
There are no perequisite dependencies for building the library on Linux since all dependencies, including Vulkan, 
are going to be managed via an isolated `conda` environment.

On Windows, MSVC version `>=19.39` must be installed via Visual Studio (version `>=17.9.7`). The library only needs the VS 
[BuildTools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) with the following components 
in the *Desktop development C++* workload selected:
- MSVC v143 - VS2022 C++ x86/64 build tools
- Windows SDK (either 10 or 11)

Set up the environment with `conda`/`mamba`:
```shell
conda env create --file environment.yml
conda activate torpedo
```

### Release build
Build the library with Clang:
```shell
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja
cmake --build build
```

Install the library into the `conda` environment:
```shell
cmake --install build
```

Downstream applications can now consume the library directly as long as they use CMake in the `conda` environment to 
configure their projects.

### Debug build
Build the library in `Debug` mode:
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