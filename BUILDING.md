`Visual Studio 2026` is required to build. As a bare minumum you must have:

- Desktop development with C++ workload
- `v145` toolset
- `ATL` for the `v145` toolset
- Windows 11 SDK

First, you'll need to perform a recursive clone of the repository. From a `PowerShell` prompt:
```
git clone --recurse https://github.com/marc2k3/foo_spider_monkey_panel
```

3rd party dependencies are installed via [vcpkg](https://github.com/microsoft/vcpkg). If you don't already haave it installed:
```
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg
./vcpkg integrate install
```

Now browse inside the `foo_spider_monkey_panel` folder and open `src/foo_spider_monkey_panel.sln` in
`Visual Studio 2026` and it should build.

When building succeeds, check the `component` folder as the compiled `dll` is copied there.
