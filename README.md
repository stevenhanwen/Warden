# Warden (macOS)

Warden is a macOS command-line tool that scans running processes and automatically groups them based on appliciations, rather than the standard activity monitor, which can show many ambiguous processes. This tool directly indicates the number of processes and total memory usage for each application, allowing a user to quickly identify what is consuming the most resources. 

## Features

- Scans all running processses on macOS
- Groups processes by application (e.g., combines helpers and the main application itself)
- Summarizes memory usage and process counts per app
- Simple and easy to use command-line interface

> More extensive work is in progress.

## Build and run

```sh
make
make run
make clean
```
