## Build instructions
All dependencies, including Vulkan, can be managed via an isolated `conda` environment:
```shell
conda env create --file environment.yml
conda activate torpedo
```

Build the library in `Release` mode:
```shell
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja
cmake --build build
```

For `Debug` build with Vulkan validation layers enabled, additional packages are required:
```shell
conda install -c conda-forge lldb=19.1.7 vulkan-validation-layers=1.4.304
```

Build the library in `Debug` mode:
```shell
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja
cmake --build build
```

Since we're not using LunarG's VulkanSDK, the `VK_LAYER_PATH` environment variable must be set so that the Vulkan loader
knows where to find the layers:
```shell
# Windows
$env:VK_LAYER_PATH="$env:CONDA_PREFIX/Library/bin"

# Linux
export VK_LAYER_PATH=$CONDA_PREFIX/Library/bin
```

> [!IMPORTANT]
> During `Debug` development, the `VK_LAYER_PATH` environment variable is ignored if running with elevated privileges, 
> see the [docs](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md) for more information.
