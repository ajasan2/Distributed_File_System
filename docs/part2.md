# Part 2: Completing the Distributed File System (DFS)

Part 2 applies a weakly consistent cache strategy to the RPC calls you already created in Part 1. This is similar to the approach used by the [Andrew File System (AFS)](https://en.wikipedia.org/wiki/Andrew_File_System).


## Goals

The gola of part 2 is to enforce a cache consisteny model on top of the current part 1 implementation.
* The consistency model adheres to whole-file caching, one Creator/Writer per file, and date based sequences.
* Checksums were employed to ensure the integrity of data during transmission and storage.


#### Source code file descriptions:

* Same as part 1. Again, majority of the files in part 2 were comepletely taken out.