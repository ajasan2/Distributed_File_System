# Part 1: Building the RPC protocol service

Part 1 implements remote procedure calls (RPC) and message types that will fetch, store, list, and get attributes for files on a remote server. RPCs were implemented using [gRPC](https://grpc.io/) for RPC services and [Protocol Buffers](https://developers.google.com/protocol-buffers/) as the definition language.

## Goals

The goal of part 1 is to generate an RPC service that will perform the following operations on a file:
* Fetch a file from a remote server and transfer its contents via gRPC
* Store a file to a remote server and transfer its data via gRPC
* List all files on the remote server and their attributes.

#### Source code file descriptions:

* `dfs-service.proto` - **TO BE MODIFIED** Holds proto buffer service and message types to this file, then run the `make protos` command to generate the source.

* `dfslib-servernode-p1.[cpp,h]` - **TO BE MODIFIED** - Override your gRPC service methods in this file by adding them to the `DFSServerImpl` class.

* `dfslib-clientnode-p1.[cpp,h]` - **TO BE MODIFIED** -  Add your client-side calls to the gRPC service in this file.

* The complete project implementation involves additional files that were purposely readacted for privacy purposes.
