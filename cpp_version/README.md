
Quick start

```bash

mkdir build
cd build
cmake ..
make
./niffer
```

See `build/report.csv` for results, each row represents a flow.

Change `cpp_version/Utility/debug.h` to modify data input

```bash
#define FILE_IN ("data_source/hadoop15.csv")
```