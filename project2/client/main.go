// A Go mirror of libfuse's hello.c

package main

import (
	"bytes"
	"flag"
	"log"
	"os"
	"path/filepath"

	ctx "golang.org/x/net/context"
	"google.golang.org/grpc"

	"github.com/hanwen/go-fuse/fuse"
	"github.com/hanwen/go-fuse/fuse/nodefs"
	"github.com/hanwen/go-fuse/fuse/pathfs"

	proto "rpc/project2/proto"
)

type HelloFs struct {
	pathfs.FileSystem
	Client proto.BasicFileServiceClient
	Files  map[string]proto.File
}

func (me *HelloFs) GetAttr(name string, context *fuse.Context) (*fuse.Attr, fuse.Status) {
	log.Println("GetAttr:", name)
	path := proto.Path{Data: name}
	fileInfo, err := me.Client.GetFileInfo(ctx.Background(), &path)
	log.Println("GetFileInfo:", fileInfo)
	if err != nil {
		log.Println(err)
		return nil, fuse.ENOENT
	}

	var mode uint32
	if name == "" { // hack for server did not return mode
		mode = fuse.S_IFDIR | 0644
	} else {
		mode = fuse.S_IFREG | 0644
	}
	return &fuse.Attr{
		Size:  fileInfo.Size,
		Atime: fileInfo.AccessTime,
		Mtime: fileInfo.ModificationTime,
		Ctime: fileInfo.CreationTime,
		Mode:  mode,
	}, fuse.OK
}

func (me *HelloFs) OpenDir(name string, context *fuse.Context) (c []fuse.DirEntry, code fuse.Status) {
	log.Println("OpenDir:", name)
	path := proto.Path{Data: name}
	dirInfo, err := me.Client.GetDirectoryContents(ctx.Background(), &path)
	if err != nil {
		log.Fatal(err)
	}
	for _, fileInfo := range dirInfo.Contents {
		//log.Println(name, fileInfo)
		c = append(c, fuse.DirEntry{Name: fileInfo})
	}
	return c, fuse.OK
}

func (me *HelloFs) Open(name string, flags uint32, context *fuse.Context) (file nodefs.File, code fuse.Status) {
	log.Println("Open:", name)
	path := proto.Path{Data: name}
	f, err := me.Client.DownloadFile(ctx.Background(), &path)
	if err != nil {
		log.Println(err)
		return nil, fuse.ENOENT
	}
	if flags&fuse.O_ANYWRITE != 0 {
		return nil, fuse.EPERM
	}

	var buf bytes.Buffer
	for _, d := range f.Contents {
		buf.WriteString(d)
	}
	return nodefs.NewDataFile(buf.Bytes()), fuse.OK
}

//func (me *HelloFs) Create(name string, flags uint32, mode uint32, context *fuse.Context) (code fuse.Status) {
//	return fuse.OK
//}

// NewGrpcClient ...
func NewGrpcCliet(ip string) proto.BasicFileServiceClient {
	conn, err := grpc.Dial(ip, grpc.WithInsecure())
	if err != nil {
		log.Fatalf("could not connect to %v: %v", ip, err)
	}
	//defer conn.Close()
	c := proto.NewBasicFileServiceClient(conn)
	return c
}

func recover(path string, info os.FileInfo, err error) error {
	log.Println(path, err, "need recover")
	return nil
}

func diagnose(cachePath string) error {
	log.Printf("Checking cache folder: %v...", cachePath)
	err := filepath.Walk(cachePath, recover)
	log.Println("Recovery check finished")
	return err
}

func main() {
	var mountPath, cachePath, ip string
	flag.StringVar(&mountPath, "m", "root", "path you want to mount AFS")
	flag.StringVar(&cachePath, "c", "cache", "cache files for write operations")
	flag.StringVar(&ip, "server", "localhost:61512", "server address")
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
