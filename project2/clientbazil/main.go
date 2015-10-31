// Hellofs implements a simple "hello world" file system.
package main

import (
	"bytes"
	"flag"
	"fmt"
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
	defer c.Close()
	if err != nil {
		log.Fatal(err)
	}
	err = diagnose(cachePath)
	if err != nil {
		log.Println(err)
	}
	log.Printf("MountPath: %s, CachePath: %s\n", mountPoint, cachePath)

	err = fs.Serve(c, &MyFS{
		cachePath:  cachePath,
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
	cachePath  string
	Client     proto.BasicFileServiceClient
	FileCaches map[string]*MyFile
}

func (f *MyFS) Root() (fs.Node, error) {
	return &MyFile{
		Name:   "/",
		parent: nil,
		fs:     f,
		buf:    bytes.NewBuffer([]byte{})}, nil
}

func (f *MyFile) Lookup(ctx context.Context, name string) (fs.Node, error) {
	filePath := filepath.Join(f.getPath(), name)
	_, ok := f.fs.FileCaches[filePath]
	if !ok {
		path := proto.Path{Data: filePath}
		fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
		if err != nil || fileInfo.ErrorCode != 0 {
			log.Println("Lookup fail\t", filePath, err, fileInfo)
			return nil, fuse.ENOENT
		}
		log.Println("MyFileLookup\tInitializing\t", filePath, "response:\t", fileInfo)
		f.fs.FileCaches[filePath] = &MyFile{
			needDownload: true,
			Name:         name,
			parent:       f,
			fs:           f.fs,
			buf:          bytes.NewBuffer([]byte{}),
			File:         &proto.File{Info: fileInfo}}
	}
	log.Printf("Lookup\t%v\t%v", filePath, f.fs.FileCaches[filePath])
	return f.fs.FileCaches[filePath], nil
}

func (f *MyFile) ReadDirAll(ctx context.Context) (c []fuse.Dirent, err error) {
	path := proto.Path{Data: f.getPath()}
	dirInfo, err := f.fs.Client.GetDirectoryContents(context.Background(), &path)

	if err != nil {
		log.Fatal(err)
	}
	for _, fileInfo := range dirInfo.Contents {
		//log.Println(name, fileInfo)
		c = append(c, fuse.Dirent{Name: fileInfo})
	}
	log.Println("ReadDirAll:", f, dirInfo.Contents)
	return c, err
}

// File implements both Node and Handle for the hello file.
type MyFile struct {
	*proto.File
	Name         string
	parent       *MyFile
	fs           *MyFS
	buf          *bytes.Buffer
	needDownload bool
}

func (f MyFile) getPath() string {
	if f.parent == nil {
		return f.Name
	}
	return filepath.Join(f.parent.getPath(), f.Name)
}

func (f *MyFile) Attr(ctx context.Context, a *fuse.Attr) error {
	filePath := f.getPath()
	_, ok := f.fs.FileCaches[filePath]
	if !ok || f.File == nil || f.File.Info == nil {
		path := proto.Path{Data: filePath}
		fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
		if err != nil {
			log.Println(err)
			return fuse.ENOENT
		}
		f.File = &proto.File{Info: fileInfo}
		f.fs.FileCaches[filePath] = f
		log.Println("Attr:\tAttrFetched\t", fileInfo, f)
	} else {
		log.Println("Attr:\tHasCache", filePath, f.File)
	}
	a.Inode = f.File.Info.Inode
	if f.File.Info.Mode&0170000 == 0040000 {
		a.Mode = os.ModeDir | 0644
	} else { // by default is regulare file, include newly created file
		a.Mode = 0644
	}
	//a.Mode = os.FileMode(uint32(f.File.Info.Mode) | 0644)
	//a.Mode = os.FileMode(uint32(f.File.Info.Mode))
	log.Println(filePath, "IsDir:", a.Mode.IsDir(), a.Mode.IsRegular(), a.Mode, f.File.Info.Mode, uint32(a.Mode))
	a.Uid = 20001 // zhaoyu
	a.Size = f.File.Info.Size
	a.Atime = time.Unix(int64(f.File.Info.AccessTime), 0)
	a.Mtime = time.Unix(int64(f.File.Info.ModificationTime), 0)
	a.Ctime = time.Unix(int64(f.File.Info.CreationTime), 0)
	f.File.Info.AccessTime = uint64(time.Now().Unix())
	return nil
}

// only for opened file or directory
func (f *MyFile) ReadAll(ctx context.Context) ([]byte, error) {
	log.Println("ReadAll:", f.getPath(), f.buf.Len(), f)
	f.File.Info.AccessTime = uint64(time.Now().Unix())
	return f.buf.Bytes(), nil
}

func (f *MyFile) Read(ctx context.Context, req *fuse.ReadRequest, resp *fuse.ReadResponse) error {
	log.Println("FileRead:", f.getPath())
	f.File.Info.AccessTime = uint64(time.Now().Unix())
	return nil
}

// only for opened file or directory
func (f *MyFile) Write(ctx context.Context, req *fuse.WriteRequest, resp *fuse.WriteResponse) error {
	tmp := f.buf.Bytes()
	f.buf.Reset()
	if int(req.FileFlags) == os.O_WRONLY {
		log.Println(req.FileFlags, req.Flags, "WriteOnly")
		f.buf.Write(req.Data)
	} else {
		log.Println(req.FileFlags, req.Flags, "ReadWrite")
		f.buf.Write(tmp[:req.Offset])
		f.buf.Write(req.Data)
		if int(req.Offset)+len(req.Data) < len(tmp) {
			f.buf.Write(tmp[int(req.Offset)+len(req.Data)+1:])
		}
	}
	resp.Size = f.buf.Len()
	log.Println("Write\t", f.getPath(), req.Offset, string(req.Data), " -> ", f.buf.String())
	f.File.Info.Size = uint64(f.buf.Len())
	f.File.Info.ModificationTime = uint64(time.Now().Unix())
	return nil
}

func (f *MyFile) Open(ctx context.Context, req *fuse.OpenRequest, resp *fuse.OpenResponse) (fs.Handle, error) {
	needDownload := f.needDownload
	filePath := f.getPath()
	_, ok := f.fs.FileCaches[filePath]
	if !ok {
		log.Println("Open\tNotCached", filePath)
		f.fs.FileCaches[filePath] = f
		needDownload = true
	}
	if needDownload == false { // check last modified time
		path := proto.Path{Data: filePath}
		fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
		if err != nil {
			log.Println(err)
			return nil, fuse.ENOENT
		}
		if f.File.Info.ModificationTime < fileInfo.ModificationTime {
			needDownload = true
			log.Println("Open\tCacheExpiredFetchServer", filePath, f.File.Info.ModificationTime, fileInfo.ModificationTime)
		} else {
			log.Println("Open\tCacheValidDoLocal", filePath)
		}
	}
	if needDownload {
		path := proto.Path{Data: filePath}
		myfile, err := f.fs.Client.DownloadFile(context.Background(), &path)
		if err != nil {
			log.Println(err)
			return nil, fuse.ENOENT
		}
		f.File = myfile
		f.buf.Reset()
		_, err = f.buf.Write(myfile.Contents)
		if err != nil {
			log.Println(err)
			return nil, fuse.ENOENT
		}
		f.needDownload = false
		log.Println("Open\tFetchedServer\t", f)
	}
	f.File.Info.AccessTime = uint64(time.Now().Unix())
	resp.Handle = fuse.HandleID(f.File.Info.Inode)
	return f, nil
}

func (f *MyFile) Flush(ctx context.Context, req *fuse.FlushRequest) error {
	log.Println("Flush\tto cache folder", f)
	return nil
}

func (f *MyFile) Remove(ctx context.Context, req *fuse.RemoveRequest) error {
	filePath := filepath.Join(f.getPath(), req.Name)
	log.Printf("DirRemove\t%v", filePath)
	path := proto.Path{Data: filePath}
	result, err := f.fs.Client.RemoveFile(context.Background(), &path)
	if err != nil || result.ErrorCode != 0 {
		log.Printf("Fail to remove %v because: %v", filePath, err, result.ErrorCode)
		return err
	}
	delete(f.fs.FileCaches, filePath)
	return nil
}

func (f *MyFile) Release(ctx context.Context, req *fuse.ReleaseRequest) error {
	var in proto.FileData
	in.Path = &proto.Path{Data: f.getPath()}
	in.Contents = f.buf.Bytes()
	response, err := f.fs.Client.UploadFile(context.Background(), &in)
	if err != nil {
		log.Println(err)
		return err
	}
	log.Printf("Release\tReq:%v\tFile:%v\tResp:%v", req, f.buf.String(), response)
	return nil
}

func (f *MyFile) Create(ctx context.Context, req *fuse.CreateRequest, resp *fuse.CreateResponse) (fs.Node, fs.Handle, error) {
	filePath := filepath.Join(f.getPath(), req.Name)
	path := proto.Path{Data: filePath}
	result, err := f.fs.Client.CreateFile(context.Background(), &path)
	if err != nil || result.ErrorCode != 0 {
		log.Println("Fail to create %v because: %v", filePath, err, result.ErrorCode)
		return nil, nil, err
	}
	// get fileinfo into cache
	fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
	if err != nil || fileInfo.ErrorCode != 0 {
		log.Println("GetFileInfo fail\t", err, fileInfo)
		return nil, nil, err
	}

	f.fs.FileCaches[filePath] = &MyFile{
		needDownload: true,
		Name:         req.Name,
		parent:       f,
		fs:           f.fs,
		buf:          bytes.NewBuffer([]byte{}),
		File:         &proto.File{Info: fileInfo}}
	log.Println("FileCreate\t", f.fs.FileCaches[filePath])
	return f.fs.FileCaches[filePath], f.fs.FileCaches[filePath], nil
}

func (f *MyFile) Mkdir(ctx context.Context, req *fuse.MkdirRequest) (fs.Node, error) {
	filePath := filepath.Join(f.getPath(), req.Name)
	log.Println("Mkdir\t", f)
	path := proto.Path{Data: filePath}
	result, err := f.fs.Client.CreateDirectory(context.Background(), &path)
	if err != nil || result.ErrorCode != 0 {
		log.Println("Fail to create %v because: %v", filePath, err)
		return nil, err
	}
	// get fileinfo into cache
	fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
	if err != nil || fileInfo.ErrorCode != 0 {
		log.Println("GetFileInfo fail\t", err, fileInfo)
		return nil, err
	}

	f.fs.FileCaches[filePath] = &MyFile{
		needDownload: true,
		Name:         req.Name,
		parent:       f,
		fs:           f.fs,
		buf:          bytes.NewBuffer([]byte{}),
		File:         &proto.File{Info: fileInfo}}
	return f.fs.FileCaches[filePath], nil
}
func (f *MyFile) Mknod(ctx context.Context, req *fuse.MknodRequest) (fs.Node, error) {
	fmt.Errorf("Mknod\t%v", f)
	return nil, nil
}
