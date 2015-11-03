// Hellofs implements a simple "hello world" file system.
package main

import (
	"bytes"
	"encoding/base64"
	"encoding/gob"
	"flag"
	"fmt"
	"hash/crc32"
	"io/ioutil"
	"log"
	"math"
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

func (f *MyFS) recover(path string, info os.FileInfo, err error) error {
	if path == f.cachePath { // hack
		return nil
	}
	if err != nil {
		log.Println("CacheFolderWalkError\t", path, info, err)
		return err
	}

	fileName, err := base64.StdEncoding.DecodeString(info.Name())
	if err != nil {
		log.Println("FailDecodePath", path, info.Name(), err)
		return err
	}
	var buf bytes.Buffer
	var brokenFile MyFile
	data, err := ioutil.ReadFile(path)
	if err != nil {
		log.Println("FailReadCache", path, string(fileName), err)
		return err
	}
	n, err := buf.Write(data)
	if err != nil || n < len(data) {
		log.Println("FailWriteBuf", path, string(fileName), err)
		return err
	}
	dec := gob.NewDecoder(&buf)
	if err := dec.Decode(&brokenFile); err != nil {
		log.Println("FailDecodeFile", path, string(fileName), err)
		return err
	}
	brokenFile.fs = f
	brokenFile.buf.Reset()
	if n, err := brokenFile.buf.Write(brokenFile.Data); err != nil || n < len(brokenFile.Data) {
		log.Println("FileCopyError", err, brokenFile.buf)
		return err
	}
	log.Println("Recovered:", path, string(fileName), info.Size(), brokenFile.File, brokenFile.buf.String(), brokenFile.Parent)
	f.FileCaches[brokenFile.FilePath] = &brokenFile

	// remove after success
	if !doCrash {
		if err := os.Remove(path); err != nil {
			log.Println("FailInRemove:", path)
			return err
		}
	}
	return nil
}

func (f *MyFS) diagnose() error {
	log.Printf("Checking cache folder: %v...", f.cachePath)
	err := filepath.Walk(f.cachePath, f.recover)
	if err != nil {
		log.Fatal(err)
		return err
	}
	log.Println("Recovery check finished")
	return nil
}

var doCrash bool

func main() {
	var mountPoint, cachePath, ip string
	flag.StringVar(&mountPoint, "m", "root", "path you want to mount AFS")
	flag.StringVar(&cachePath, "c", "cache", "cache files for write operations")
	flag.StringVar(&ip, "server", "localhost:61512", "server address")
	flag.BoolVar(&doCrash, "crash", false, "do crash demo or not:\nthis would disable donwload files from server and prevent delete cache files")
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
	log.Printf("MountPath: %s, CachePath: %s\n", mountPoint, cachePath)
	myfs := &MyFS{
		cachePath:  cachePath,
		Client:     NewGrpcClient(ip),
		FileCaches: make(map[string]*MyFile)}
	err = myfs.diagnose()
	if err != nil {
		log.Println(err)
	}

	err = fs.Serve(c, myfs)
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
	root       *MyFile
	cachePath  string
	Client     proto.BasicFileServiceClient
	FileCaches map[string]*MyFile
}

func (f *MyFS) Root() (fs.Node, error) {
	if f.root == nil {
		root := &MyFile{
			Name:   "/",
			Parent: nil,
			fs:     f,
			buf:    *bytes.NewBuffer([]byte{})}
		f.root = root
	}
	return f.root, nil
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
		log.Println("Lookup:", filePath, "\tinit&response:", fileInfo)
		f.fs.FileCaches[filePath] = &MyFile{
			NeedDownload: true,
			Name:         name,
			Parent:       f,
			fs:           f.fs,
			buf:          *bytes.NewBuffer([]byte{}),
			File:         &proto.File{Info: fileInfo}}
	}
	log.Printf("Lookup:%v\t%v", filePath, f.fs.FileCaches[filePath].File.Info)
	return f.fs.FileCaches[filePath], nil
}

func (f *MyFile) ReadDirAll(ctx context.Context) (c []fuse.Dirent, err error) {
	path := proto.Path{Data: f.getPath()}
	dirInfo, err := f.fs.Client.GetDirectoryContents(context.Background(), &path)

	if err != nil {
		log.Fatal(err)
	}
	for _, fileInfo := range dirInfo.Contents {
		c = append(c, fuse.Dirent{Name: fileInfo})
	}
	log.Println("ReadDirAll:", f.getPath(), dirInfo.Contents)
	return c, err
}

// File implements both Node and Handle for the hello file.
type MyFile struct {
	File         *proto.File
	Name         string
	Data         []byte
	FilePath     string
	Parent       *MyFile
	fs           *MyFS
	buf          bytes.Buffer
	NeedDownload bool
	LastChecksum uint32
}

type MyFileData struct {
}

func (f MyFile) getPath() string {
	if f.Parent == nil {
		return f.Name
	}
	return filepath.Join(f.Parent.getPath(), f.Name)
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
		log.Println("Attr:", filePath, "\tAttrFetched\t", f.File.Info)
	} else {
		log.Println("Attr:", filePath, "\tHasCache\tMTime:", f.File.Info.ModificationTime)
	}
	a.Inode = f.File.Info.Inode
	if f.File.Info.Mode&0170000 == 0040000 {
		a.Mode = os.ModeDir | 0644
	} else { // by default is regulare file, include newly created file
		a.Mode = 0644
	}
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
	log.Println("ReadAll:", f.getPath(), f.buf.Len())
	f.File.Info.AccessTime = uint64(time.Now().Unix())
	return f.buf.Bytes(), nil
}

func (f *MyFile) Read(ctx context.Context, req *fuse.ReadRequest, resp *fuse.ReadResponse) error {
	log.Println("FileRead:", f.getPath())
	f.File.Info.AccessTime = uint64(time.Now().Unix())
	return nil
}

func WriteAt(buf *bytes.Buffer, data []byte, offset int) error {
	if offset > buf.Len() {
		return fmt.Errorf("offset %v > buf %v: %v", offset, buf, data)
	}
	oldLength := buf.Len()
	if offset+len(data) > oldLength { // enlarge the file
		fmt.Println(offset+len(data)-oldLength, oldLength, offset, len(data))
		n, err := buf.Write(data[oldLength-offset:]) // write the back first to enlarge the buf size
		if err != nil || n != offset+len(data)-oldLength {
			fmt.Errorf("%v\t%v\toffset+length:%v\tlength-offset:%v", err, data, offset+len(data), oldLength-offset)
			return err
		}
	}
	tmp := buf.Bytes()
	copy(tmp[offset:oldLength], data[:int(math.Min(float64(oldLength-offset), float64(len(data))))])
	return nil
}

// only for opened file or directory
func (f *MyFile) Write(ctx context.Context, req *fuse.WriteRequest, resp *fuse.WriteResponse) error {
	if req.FileFlags == fuse.OpenReadWrite || req.FileFlags == fuse.OpenWriteOnly {
		err := WriteAt(&f.buf, req.Data, int(req.Offset))
		log.Println("Write:", f.getPath(), req.FileFlags, req.Flags, req.Offset, len(req.Data), " -> ", f.buf.Len())
		if err != nil {
			return fuse.EIO
		}
	} else {
		log.Println("Write:", f.getPath(), "\tStrangeFileFlags\t", req.FileFlags, req.Flags, req.Offset, len(req.Data))
	}
	resp.Size = len(req.Data)
	f.File.Info.Size = uint64(f.buf.Len())
	f.File.Info.ModificationTime = uint64(time.Now().Unix())
	return nil
}

func (f *MyFile) Open(ctx context.Context, req *fuse.OpenRequest, resp *fuse.OpenResponse) (fs.Handle, error) {
	var msg string
	startTime := time.Now()
	NeedDownload := f.NeedDownload
	filePath := f.getPath()
	_, ok := f.fs.FileCaches[filePath]
	if !ok {
		msg = "NotCached"
		f.fs.FileCaches[filePath] = f
		NeedDownload = true
	}
	if NeedDownload == false { // check last modified time
		path := proto.Path{Data: filePath}
		fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
		if err != nil {
			log.Println(err)
			return nil, fuse.ENOENT
		}
		if f.File.Info.ModificationTime < fileInfo.ModificationTime {
			NeedDownload = true
			msg = "CacheExpiredFetchServer"
		} else {
			msg = "CacheValidNoDownload"
		}
	}
	if NeedDownload && !doCrash {
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
		f.NeedDownload = false
		// the only place needs to calculate crc32
		f.LastChecksum = crc32.ChecksumIEEE(f.buf.Bytes())
		msg = "FetchedServer"
	}
	f.File.Info.AccessTime = uint64(time.Now().Unix())
	resp.Handle = fuse.HandleID(f.File.Info.Inode)
	log.Printf("Open: %v\t%v\t%v\t%v", filePath, time.Now().Sub(startTime), msg, f.LastChecksum)
	return f, nil
}

func (f *MyFile) Flush(ctx context.Context, req *fuse.FlushRequest) error {
	filePath := f.getPath()
	cacheFileName := base64.StdEncoding.EncodeToString([]byte(filePath))
	cacheFilePath := filepath.Join(f.fs.cachePath, cacheFileName)
	f.Data = f.buf.Bytes()
	f.FilePath = filePath
	var buf bytes.Buffer
	enc := gob.NewEncoder(&buf)
	if err := enc.Encode(f); err != nil {
		log.Println(err)
		return err
	}
	err := ioutil.WriteFile(cacheFilePath, buf.Bytes(), 0644)
	if err != nil {
		log.Println(err)
		return err
	}
	log.Println("Flush:", f.getPath(), f.buf.Len(), "\tflag:", req.Flags, "\tTo", cacheFileName, buf.Len())
	return nil
}

func (f *MyFile) Remove(ctx context.Context, req *fuse.RemoveRequest) error {
	filePath := filepath.Join(f.getPath(), req.Name)
	log.Printf("DirRemove:%v", filePath)
	path := proto.Path{Data: filePath}
	result, err := f.fs.Client.RemoveFile(context.Background(), &path)
	if err != nil || result.ErrorCode != 0 {
		log.Printf("Fail to remove %v because: %v %v", filePath, err, result.ErrorCode)
		return err
	}
	delete(f.fs.FileCaches, filePath)
	return nil
}

func (f *MyFile) Release(ctx context.Context, req *fuse.ReleaseRequest) error {
	filePath := f.getPath()
	path := proto.Path{Data: filePath}
	fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
	if err != nil {
		log.Println(err)
		return fuse.ENOENT
	}

	newChecksum := crc32.ChecksumIEEE(f.buf.Bytes())
	if f.File.Info.ModificationTime < fileInfo.ModificationTime {
		log.Printf("Release:%v\tWARNING\tLocalFileExpireNoUpload", filePath)
	} else {
		if newChecksum == f.LastChecksum {
			log.Printf("Release:%v\tNoChangeNoUpload\t%v\t%v\t%v", filePath, f.buf.Len(), f.LastChecksum, newChecksum)
		} else {
			var in proto.FileData
			in.Path = &proto.Path{Data: filePath}
			in.Contents = f.buf.Bytes()
			response, err := f.fs.Client.UploadFile(context.Background(), &in)
			if err != nil || response.ErrorCode != 0 {
				log.Println(err, response)
				return err
			}
			f.LastChecksum = newChecksum
			log.Printf("Release:%v\tUpload %v B", filePath, f.buf.Len())
		}
	}
	return nil
}

func (f *MyFile) Create(ctx context.Context, req *fuse.CreateRequest, resp *fuse.CreateResponse) (fs.Node, fs.Handle, error) {
	filePath := filepath.Join(f.getPath(), req.Name)
	path := proto.Path{Data: filePath}
	result, err := f.fs.Client.CreateFile(context.Background(), &path)
	if err != nil || result.ErrorCode != 0 {
		log.Printf("Fail to create %v because: %v %v", filePath, err, result.ErrorCode)
		return nil, nil, err
	}
	// fetch fileinfo into cache
	fileInfo, err := f.fs.Client.GetFileInfo(context.Background(), &path)
	if err != nil || fileInfo.ErrorCode != 0 {
		log.Println("GetFileInfo fail\t", filePath, err, fileInfo)
		return nil, nil, err
	}

	f.fs.FileCaches[filePath] = &MyFile{
		NeedDownload: true,
		Name:         req.Name,
		Parent:       f,
		fs:           f.fs,
		buf:          *bytes.NewBuffer([]byte{}),
		File:         &proto.File{Info: fileInfo}}
	log.Println("FileCreate:", filePath, f.fs.FileCaches[filePath])
	return f.fs.FileCaches[filePath], f.fs.FileCaches[filePath], nil
}

func (f *MyFile) Mkdir(ctx context.Context, req *fuse.MkdirRequest) (fs.Node, error) {
	filePath := filepath.Join(f.getPath(), req.Name)
	log.Println("Mkdir:", filePath)
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
		NeedDownload: true,
		Name:         req.Name,
		Parent:       f,
		fs:           f.fs,
		buf:          *bytes.NewBuffer([]byte{}),
		File:         &proto.File{Info: fileInfo}}
	return f.fs.FileCaches[filePath], nil
}
func (f *MyFile) Mknod(ctx context.Context, req *fuse.MknodRequest) (fs.Node, error) {
	fmt.Errorf("Mknod:%v", f.getPath())
	return nil, nil
}

func (f *MyFile) Fsync(ctx context.Context, req *fuse.FsyncRequest) error {
	fmt.Errorf("Fsync:%v", f.getPath())
	return nil
}
