## Run tests
```shell
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build
```