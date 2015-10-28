# rpc
cs 739 intro to communication

## Install
1. `go get google.golang.org/grpc`
- `git clone https://github.com/grpc/grpc.git`
    * `git submodule update --init`
    * `make`
    * `make install prefix=$HOME/local`
- ensure `protoc-gen-go` and `grpc_cpp_plugin` are present

### gRPC Support
`protoc --go_out=plugins=grpc:. *.proto`

## [Crash-Consistent] (https://www.usenix.org/system/files/conference/osdi14/osdi14-paper-pillai.pdf)
* break down categories:
    * atomicity
    * ordering of operations: does file system ensure file creations are persisted in the same order?

### FS operations major categories
* file overwrite
* file append
* directory operations
    * rename
    * link
    * unlink
    * mkdir

### [Presentation Video] (https://www.usenix.org/conference/osdi14/technical-sessions/presentation/pillai)
* WriteAheadLog
* ext3(data-ordered) need `fsync` to ensure order, prevent FS re-order

    creat(/x/log1);
    write(/x/log1, "2, 3, checksum, foo"); // checksum is to prevent garbage, which means file size alone increases
    fsync(/x/log1);
    fsync(/x); // ensure log file's directory entry might never be created
    pwrite(/x/f1, 2, "bar");
    fsync(/x/f1);
    unlink(/x/log1);
    fsync(/x); // additional fsync() required for durability in all FS

### Our approach
* Ensure crash-consistent inside `write()`
    1. compute file checksum
    - write checksum: 32 bits, filecontent into `cache/filepath_base64`
        * if cache file exists, over-write it
    - `fsync()` cache file into file system
	- we may add a hook here to cause system exit before write into FUSE
	    * goroutine sends a channel signal
    - actual write into FUSE
    - delete cache file
    - return
* crash recovery upon startup
    1. see whether there are files inside cache folder
    - decode files and apply to FUSE
    - start normal service
