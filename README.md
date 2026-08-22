# Pixel Storm

## Development environment setup

### MacOS

- `.vscode/settings.json`

```json
{
    "cmake.debugConfig": {
        "cwd": "${workspaceFolder}",
        "args": [
            "--image",
            "assets/skull.png",
            "--gap",
            "4"
        ]
    },
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
}
```

- `.vscode/c_cpp_properties.json`

```json
{
    "configurations": [
        {
            "name": "CMake",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json",
            "compilerPath": "/usr/bin/clang++",
            "cppStandard": "c++23",
            "cStandard": "c99",
            "includePath": [
                "${workspaceFolder}/src",
                "${workspaceFolder}/vendor/stb",
                "${workspaceFolder}/vendor/log"
            ]
        }
    ],
    "version": 4
}
```

## Benchmarking

```bash
./build/Pixel_Storm -i assets/benchmarks/<benchmark_image.png> -g 1 --benchmark <output_path.csv> --uncapped
```

- Simple benchmark done in Release mode.