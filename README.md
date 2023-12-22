# Distributed File System

## Important Note

Please note that some code segments are intentionally redacted in compliance with school policy, with descriptions provided in their place. This information is shared for the purpose of demonstrating technical skills to potential employers and should not be publicly disclosed. Please be advised that this repository may only be available temporarily. Prospective employers are welcome to request access to the comprehensive codebase directly from the author.

The documentation intentionally omits detailed descriptions of functions and maintains a general project structure in this public repository. This approach is adopted to uphold the integrity of the instructional staff's efforts and prevent unauthorized duplication of the project.

## Project Overview

This project designs and implements a simple distributed file system (DFS) using remote procedure calls (RPC).  It also incorporates a weakly consistent synchronization system to manage cache consistency between multiple clients and a single server. The system can handle both binary and text-based files.

## Setup

Base dependencies involve a combination of C++14, gRPC, and Protocol Buffers. `inotify` was used as a Linux kernel subsystem for real-time monitoring of file system events.

## Project Approach

Throughout the project, the author has recorded approaches to each part of the project and the resources used. This can be made available upon request to the author.

## Component Overview

- Part 1 can be found in [docs/part1.md](docs/part1.md)
- Part 2 can be found in [docs/part2.md](docs/part2.md)

## Program Log

A logging utility, with various log levels, is used to track program execution and for debugging purposes.