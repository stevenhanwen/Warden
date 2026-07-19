# Warden (macOS)

Warden is a macOS command-line tool that scans running processes and automatically groups them by application, rather than the standard Activity Monitor view, which can show many ambiguous processes. Warden directly indicates the number of processes and total memory usage for each application, allowing you to quickly identify what is consuming the most resources. It also offers a much quicker and more efficient experience for terminating processes compared to using a GUI.

## Features

- Scans all running processes on macOS
- Groups processes by application (e.g., combines helpers and the main application itself)
- Summarizes memory usage and process counts per app
- Search mode for filtering by process name
- Simple and easy to use command-line interface

> More extensive work is in progress.

---

## Using Warden

### Install and run via Homebrew

```sh
brew install stevenhanwen/warden/warden
warden
```

### Update

```sh
brew upgrade warden
```

### Uninstall

```sh
brew uninstall warden
```

### Note on permissions

The first time you install or run Warden, macOS may prompt for your password (for Xcode Command Line Tools / Homebrew setup) and possibly for additional system permissions related to inspecting or terminating other processes. This is expected and only needs to happen once.

---

## Contributing

### Build and run (CMake)

```sh
git clone https://github.com/stevenhanwen/warden.git
cd warden

# Re-run this only if you change CMakeLists.txt
cmake -S . -B build

cmake --build build
./build/warden
```

### Run tests (GoogleTest + CTest)

```sh
# Configure/build (if not already done)
cmake -S . -B build
cmake --build build

# Run all discovered tests
ctest --test-dir build --output-on-failure

# Optional: run the test binary directly
./build/warden_tests
```