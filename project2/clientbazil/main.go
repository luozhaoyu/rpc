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
	//	return &MyFile{Name: name, needDownload: true, buf: bytes.NewBuffer([]byte{}), fs: d.fs}, nil
	_, ok := d.fs.FileCaches[name]
	if !ok {
		log.Println("Lookup\tInitializing\t", name)
		d.fs.FileCaches[name] = &MyFile{needDownload: true, Name: name, fs: d.fs, buf: bytes.NewBuffer([]byte{})}
	}
	log.Printf("Lookup\t%v\t%v", name, d.fs.FileCaches[name])
	return d.fs.FileCaches[name], nil

	fileCache, ok := d.fs.FileCaches[name]
	if !ok {
		log.Println("Lookup:\tFetchInfoFromServer", name)
		path := proto.Path{Data: name}
		fileInfo, err := d.fs.Client.GetFileInfo(context.Background(), &path)
		if err != nil {
			log.Println(err)
			return nil, fuse.ENOENT
		}
		fileCache = &MyFile{needDownload: true, Name: name, fs: d.fs, buf: bytes.NewBuffer([]byte{})}
		log.Println(fileCache, "is alive")
		fileCache.File = &proto.File{Info: fileInfo}
		d.fs.FileCaches[name] = fileCache
	}
	log.Println("Lookup:", d.fs.FileCaches[name])
	return d.fs.FileCaches[name], nil
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
	Name         string
	fs           *MyFS
	buf          *bytes.Buffer
	needDownload bool
}

func (f *MyFile) Attr(ctx context.Context, a *fuse.Attr) error {
	_, ok := f.fs.FileCaches[f.Name]
	if !ok || f.File == nil || f.File.Info == nil {
		log.Println("Attr:\tNoCache", f.Name)
		path := proto.Path{Data: f.Name}
		fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
		if err != nil {
			log.Println(err)
			return fuse.ENOENT
		}
		f.File = &proto.File{Info: fileInfo}
		f.fs.FileCaches[f.Name] = f
	} else {
		log.Println("Attr:\tHasCache", f.Name, f.File)
	}
	a.Inode = f.File.Info.Inode
	a.Mode = os.FileMode(uint32(f.File.Info.Mode))
	a.Size = f.File.Info.Size
	a.Atime = time.Unix(int64(f.File.Info.AccessTime), 0)
	a.Mtime = time.Unix(int64(f.File.Info.ModificationTime), 0)
	a.Ctime = time.Unix(int64(f.File.Info.CreationTime), 0)
	return nil
}

// only for opened file or directory
func (f *MyFile) ReadAll(ctx context.Context) ([]byte, error) {
	log.Println("ReadAll:", f.Name, f.buf.Len(), f)
	return f.buf.Bytes(), nil
}

func (f *MyFile) Read(ctx context.Context, req *fuse.ReadRequest, resp *fuse.ReadResponse) error {
	log.Println("FileRead:", f.Name)
	return nil
}

// only for opened file or directory
func (f *MyFile) Write(ctx context.Context, req *fuse.WriteRequest, resp *fuse.WriteResponse) error {
	b := f.buf.Bytes()
	tmp := append(b[:req.Offset], req.Data...)
	if int(req.Offset)+len(req.Data) < len(b) {
		tmp = append(tmp, b[int(req.Offset)+len(req.Data)+1:]...)
	}
	b = tmp
	resp.Size = len(b)
	log.Printf("Write\t%v\t%v", f, resp.Size)
	return nil
}

func (f *MyFile) Open(ctx context.Context, req *fuse.OpenRequest, resp *fuse.OpenResponse) (fs.Handle, error) {
	needDownload := f.needDownload
	_, ok := f.fs.FileCaches[f.Name]
	if !ok {
		log.Println("Open\tNotCached", f.Name)
		f.fs.FileCaches[f.Name] = f
		needDownload = true
	}
	if needDownload == false { // check last modified time
		path := proto.Path{Data: f.Name}
		fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
		if err != nil {
			log.Println(err)
			return nil, fuse.ENOENT
		}
		if f.File.Info.ModificationTime < fileInfo.ModificationTime {
			needDownload = true
			log.Println("Open\tCacheExpired", f.Name, f.File.Info.ModificationTime, fileInfo.ModificationTime)
		} else {
			log.Println("Open\tCacheValid", f.Name)
		}
	}
	if needDownload {
		path := proto.Path{Data: f.Name}
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
		log.Println("Open\tDownloaded\t", f)
	}
	resp.Handle = fuse.HandleID(f.File.Info.Inode)
	return f, nil
}

func (f *MyFile) Flush(ctx context.Context, req *fuse.FlushRequest) error {
	var in proto.FileData
	in.Path = &proto.Path{Data: f.Name}
	in.Contents = f.buf.Bytes()
	response, err := f.fs.Client.UploadFile(context.Background(), &in)
	if err != nil {
		log.Println(err)
		return err
	}
	log.Printf("Flush\t%v\t%v", f, response)
	return nil
}

func (d *Dir) Remove(ctx context.Context, req *fuse.RemoveRequest) error {
	log.Printf("DirRemove\t%v", req.Name)
	path := proto.Path{Data: req.Name}
	result, err := d.fs.Client.RemoveFile(context.Background(), &path)
	if err != nil || result.ErrorCode != 0 {
		log.Println("Fail to remove %v because: %v", req.Name, err)
		return err
	}
	return nil
}

func (f *MyFile) Remove(ctx context.Context, req *fuse.RemoveRequest) error {
	log.Printf("Remove\t%v", f.Name)
	path := proto.Path{Data: f.Name}
	result, err := f.fs.Client.RemoveFile(context.Background(), &path)
	if err != nil || result.ErrorCode != 0 {
		log.Println("Fail to remove %v because: %v", f.Name, err)
		return err
	}
	return nil
}
