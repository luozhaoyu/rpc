// A Go mirror of libfuse's hello.c

package main

import (
	"flag"
	"log"
	"os"
	"path/filepath"

	ctx "golang.org/x/net/context"
	"google.golang.org/grpc"

	"github.com/hanwen/go-fuse/fuse"
	"github.com/hanwen/go-fuse/fuse/nodefs"
	"github.com/hanwen/go-fuse/fuse/pathfs"

	"rpc/project2/proto"
)

type HelloFs struct {
	pathfs.FileSystem
	Client File.BasicFileServiceClient
}

func (me *HelloFs) GetAttr(name string, context *fuse.Context) (*fuse.Attr, fuse.Status) {
	path := File.Path{Data: name}
	fileInfo, err := me.Client.GetFileInfo(ctx.Background(), &path)
	if err != nil {
		log.Println(err)
		return nil, fuse.ENOENT
	}
	return &fuse.Attr{
		Size:  fileInfo.Size,
		Atime: fileInfo.AccessTime,
		Mtime: fileInfo.ModificationTime,
		Ctime: fileInfo.CreationTime,
		Mode:  fuse.S_IFREG | 0644,
	}, fuse.OK
}

func (me *HelloFs) OpenDir(name string, context *fuse.Context) (c []fuse.DirEntry, code fuse.Status) {
	path := File.Path{Data: name}
	dirInfo, err := me.Client.GetDirectoryContents(ctx.Background(), &path)
	if err != nil {
		log.Fatal(err)
	}
	for _, fileInfo := range dirInfo.Contents {
		log.Println(name, fileInfo.Name)
		c = append(c, fuse.DirEntry{Name: fileInfo.Name})
	}
	return c, fuse.OK
}

func (me *HelloFs) Open(name string, flags uint32, context *fuse.Context) (file nodefs.File, code fuse.Status) {
	if name != "file.txt" {
		return nil, fuse.ENOENT
	}
	if flags&fuse.O_ANYWRITE != 0 {
		return nil, fuse.EPERM
	}
	return nodefs.NewDataFile([]byte(name)), fuse.OK
}

// NewGrpcClient ...
func NewGrpcCliet(ip string) File.BasicFileServiceClient {
	conn, err := grpc.Dial(ip, grpc.WithInsecure())
	if err != nil {
		log.Fatalf("could not connect to %v: %v", ip, err)
	}
	//defer conn.Close()
	c := File.NewBasicFileServiceClient(conn)
	return c
}

func recover(path string, info os.FileInfo, err error) error {
	log.Println(path, err, "need recover")
	return nil
}

func diagnose(cachePath string) error {
	log.Println("Checking if there was crash before...")
	err := filepath.Walk(cachePath, recover)
	log.Println("Recovery check finished")
	return err
}

func main() {
	var mountPath, cachePath, ip string
	flag.StringVar(&mountPath, "-t", "tmp", "path you want to mount AFS")
	flag.StringVar(&cachePath, "-c", "cache", "cache files for write operations")
	flag.StringVar(&ip, "-i", "localhost:31349", "server address")
	flag.Parse()

	nfs := pathfs.NewPathNodeFs(&HelloFs{
		FileSystem: pathfs.NewDefaultFileSystem(),
		Client:     NewGrpcCliet(ip)}, nil)
	server, _, err := nodefs.MountRoot(mountPath, nfs.Root(), nil)
	if err != nil {
		log.Fatalf("Mount fail: %v\n", err)
	}
	diagnose(cachePath)
	log.Printf("MountPath: %s, CachePath: %s\n", mountPath, cachePath)
	server.Serve()
}
