// Hellofs implements a simple "hello world" file system.
package main

import (
	"bytes"
	"flag"
	"log"
	"os"
	"path/filepath"
	"time"

	"golang.org/x/net/context"

	proto "rpc/project2/proto"

	"google.golang.org/grpc"

	"bazil.org/fuse"
	"bazil.org/fuse/fs"
	_ "bazil.org/fuse/fs/fstestutil"
)

func NewGrpcClient(ip string) proto.BasicFileServiceClient {
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
	var mountPoint, cachePath, ip string
	flag.StringVar(&mountPoint, "m", "root", "path you want to mount AFS")
	flag.StringVar(&cachePath, "c", "cache", "cache files for write operations")
	flag.StringVar(&ip, "server", "localhost:61512", "server address")
	flag.Parse()

	c, err := fuse.Mount(
		mountPoint,
		fuse.FSName("helloworld"),
		fuse.Subtype("hellofs"),
		fuse.LocalVolume(),
		fuse.VolumeName("Hello world!"),
	)
	if err != nil {
		log.Fatal(err)
	}
	defer c.Close()
	diagnose(cachePath)
	log.Printf("MountPath: %s, CachePath: %s\n", mountPoint, cachePath)

	err = fs.Serve(c, &MyFS{
		Client:     NewGrpcClient(ip),
		FileCaches: make(map[string]*MyFile)})
	if err != nil {
		log.Fatal(err)
	}

	// check if the mount process has an error to report
	<-c.Ready
	if err := c.MountError; err != nil {
		log.Fatal(err)
	}
}

// FS implements the hello world file system.
type MyFS struct {
	Client     proto.BasicFileServiceClient
	FileCaches map[string]*MyFile
}

func (f *MyFS) Root() (fs.Node, error) {
	return &Dir{Name: "", fs: f}, nil
}

// Dir implements both Node and Handle for the root directory.
type Dir struct {
	Name string
	fs   *MyFS
}

func MyGetAttr(Client proto.BasicFileServiceClient, name string, a *fuse.Attr) error {
	log.Println("MyGetAttr:", name)
	//a.Inode = 2
	path := proto.Path{Data: name}
	fileInfo, err := Client.GetFileInfo(context.Background(), &path)
	if err != nil {
		log.Println(err)
		return nil
	}

	if name == "" { // hack for server did not return mode
		a.Mode = os.ModeDir | 0644
	} else {
		a.Mode = 0644
	}
	a.Size = fileInfo.Size
	a.Atime = time.Unix(int64(fileInfo.AccessTime), 0)
	a.Mtime = time.Unix(int64(fileInfo.ModificationTime), 0)
	a.Ctime = time.Unix(int64(fileInfo.CreationTime), 0)

	return nil
}

func (d *Dir) Attr(ctx context.Context, a *fuse.Attr) error {
	log.Println("DirAttr:", d.Name)
	return MyGetAttr(d.fs.Client, d.Name, a)
}

func (d *Dir) Lookup(ctx context.Context, name string) (fs.Node, error) {
	log.Println("Lookup:", name)
	return &MyFile{Name: name, fs: d.fs}, nil
}

var dirDirs = []fuse.Dirent{
	{Inode: 2, Name: "hello", Type: fuse.DT_File},
}

func (d *Dir) ReadDirAll(ctx context.Context) (c []fuse.Dirent, err error) {
	log.Println("ReadDirAll:", d.Name)
	path := proto.Path{Data: d.Name}
	dirInfo, err := d.fs.Client.GetDirectoryContents(context.Background(), &path)

	if err != nil {
		log.Fatal(err)
	}
	for _, fileInfo := range dirInfo.Contents {
		//log.Println(name, fileInfo)
		c = append(c, fuse.Dirent{Name: fileInfo})
	}
	return c, err
}

//func (d *Dir) ReadDirAll(ctx context.Context) (c []fuse.Dirent, err error) {
//	log.Println(ctx, d)
//	return dirDirs, nil
//}

// File implements both Node and Handle for the hello file.
type MyFile struct {
	*proto.File
	Name string
	fs   *MyFS
	buf  bytes.Buffer
}

const greeting = "hello, world\n"

func (f *MyFile) Attr(ctx context.Context, a *fuse.Attr) error {
	log.Println("FileAttr:", f.Name)
	return MyGetAttr(f.fs.Client, f.Name, a)
}

// only for opened file or directory
func (f *MyFile) ReadAll(ctx context.Context) ([]byte, error) {
	log.Println("FileReadAll:", f.Name, f.buf.Len())
	return f.buf.Bytes(), nil
}

func (f *MyFile) Read(ctx context.Context, req *fuse.ReadRequest, resp *fuse.ReadResponse) error {
	log.Println("FileRead:", f.Name)
	return nil
}

// only for opened file or directory
func (f *MyFile) Write(ctx context.Context, req *fuse.WriteRequest, resp *fuse.WriteResponse) error {
	log.Println("FileWrite", f.Name, len(req.Data))
	b := f.buf.Bytes()
	tmp := append(b[:req.Offset], req.Data...)
	if int(req.Offset)+len(req.Data) < len(b) {
		tmp = append(tmp, b[int(req.Offset)+len(req.Data)+1:]...)
	}
	b = tmp
	resp.Size = len(b)
	return nil
}

func (f *MyFile) Open(ctx context.Context, req *fuse.OpenRequest, resp *fuse.OpenResponse) (fs.Handle, error) {
	log.Println("FileOpen", f.Name)
	path := proto.Path{Data: f.Name}
	myfile, err := f.fs.Client.DownloadFile(context.Background(), &path)
	if err != nil {
		log.Println(err)
		return nil, fuse.ENOENT
	}

	fileCache := &MyFile{File: myfile, Name: f.Name, fs: f.fs}
	for _, d := range myfile.Contents {
		fileCache.buf.WriteString(d)
	}
	f.fs.FileCaches[f.Name] = fileCache
	return fileCache, nil
}

func (f *MyFile) Flush(ctx context.Context, req *fuse.FlushRequest) error {
	log.Println("FileFlush", f.Name, f.buf.Len())
	var in proto.FileData
	in.Path = &proto.Path{Data: f.Name}
	in.Contents = append(in.Contents, f.buf.String())
	_, err := f.fs.Client.UploadFile(context.Background(), &in)
	if err != nil {
		log.Println(err)
		return err
	}
	return nil
}
