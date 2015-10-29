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
		Client: NewGrpcClient(ip),
		Files:  make(map[string]proto.File)})
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
	Client proto.BasicFileServiceClient
	Files  map[string]proto.File
}

func (f *MyFS) Root() (fs.Node, error) {
	return &Dir{Name: "", Client: f.Client}, nil
}

// Dir implements both Node and Handle for the root directory.
type Dir struct {
	Name   string
	Client proto.BasicFileServiceClient
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
	return MyGetAttr(d.Client, d.Name, a)
}

func (d *Dir) Lookup(ctx context.Context, name string) (fs.Node, error) {
	log.Println("Lookup:", name)
	return &File{Name: name, Client: d.Client}, nil
}

var dirDirs = []fuse.Dirent{
	{Inode: 2, Name: "hello", Type: fuse.DT_File},
}

func (d *Dir) ReadDirAll(ctx context.Context) (c []fuse.Dirent, err error) {
	log.Println("ReadDirAll:", d.Name)
	path := proto.Path{Data: d.Name}
	dirInfo, err := d.Client.GetDirectoryContents(context.Background(), &path)

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
type File struct {
	Name   string
	Client proto.BasicFileServiceClient
}

const greeting = "hello, world\n"

func (f *File) Attr(ctx context.Context, a *fuse.Attr) error {
	log.Println("FileAttr:", f.Name)
	return MyGetAttr(f.Client, f.Name, a)
}

func (f *File) ReadAll(ctx context.Context) ([]byte, error) {
	log.Println("FileReadAll:", f.Name)
	path := proto.Path{Data: f.Name}
	myfile, err := f.Client.DownloadFile(context.Background(), &path)
	if err != nil {
		log.Println(err)
		return nil, fuse.ENOENT
	}

	var buf bytes.Buffer
	for _, d := range myfile.Contents {
		buf.WriteString(d)
	}
	return buf.Bytes(), nil
}

func (f *File) Write(ctx context.Context, req *fuse.WriteRequest, resp *fuse.WriteResponse) error {
	log.Println("FileWrite", f.Name)
	var in proto.FileData
	in.Path = &proto.Path{Data: f.Name}
	content := string(req.Data)
	in.Contents = append(in.Contents, content)
	fileInfo, err := f.Client.UploadFile(context.Background(), &in)
	if err != nil {
		log.Println(err)
		return err
	}
	resp.Size = int(fileInfo.Size)
	return nil
}

//func (f *File) Open(ctx context.Context, req *fuse.OpenRequest, resp *fuse.OpenResponse) (fs.Handle, error) {
//	log.Println("FileOpen", f.Name)
//	return nil, nil
//}

func (f *File) Flush(ctx context.Context, req *fuse.FlushRequest) error {
	log.Println("FileFlush", f.Name)
	return nil
}
