# Warden (macOS)

Warden is a macOS command-line tool that scans running processes and automatically groups them based on applciations, rather than something messy like the built in activity monitor. It directly indicates the numebr of process and total memory usage for each application, to allow a user to quickly identify what is consuming the most resources. 

## Features

- Scans all running processses on macOS
- Groups processes by application (e.g., combines helpers and main app)
- Summarizes memory usage and process counts per app
- Simple and easy to use command-line interface

> More extensive work is in progress.

## Build and run

```sh
make
make run
make clean
```
