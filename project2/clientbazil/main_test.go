package main

import (
	"io/ioutil"
	"os"
	"os/exec"
	"testing"
)

func CheckError(err error, t *testing.T) {
	if err != nil {
		t.Log(err)
		t.FailNow()
	}
}

func TestTouchAndWriteFile(t *testing.T) {
	out, err := exec.Command("rm", "-rf", "root/*").Output()
	out, err = exec.Command("touch", "root/a").Output()
	CheckError(err, t)
	err = ioutil.WriteFile("root/a", []byte("File Creation and Write seems OK"), 0644)
	CheckError(err, t)
	out, err = exec.Command("cat", "root/a").Output()
	t.Log(string(out))
	CheckError(err, t)
}

func TestMkdir(t *testing.T) {
	out, err := exec.Command("rm", "-rf", "root/*").Output()
	CheckError(err, t)
	out, err = exec.Command("mkdir", "-p", "root/b/b").Output()
	CheckError(err, t)
	out, err = exec.Command("stat", "root/b/b").Output()
	t.Log(string(out))
	CheckError(err, t)
}

func TestReadAt(t *testing.T) {
	err := ioutil.WriteFile("root/test_read_at", []byte("123456789"), 0644)
	CheckError(err, t)
	f, err := os.OpenFile("root/test_read_at", os.O_RDWR, 0644)
	CheckError(err, t)
	var readData []byte
	var n int
	readData = make([]byte, 3)
	n, err = f.ReadAt(readData, 1)
	t.Log("ReadAt", n, err, string(readData))
	CheckError(err, t)
	if n != len(readData) || string(readData) != "234" {
		t.FailNow()
	}

}

func TestWriteAt(t *testing.T) {
	var n int
	fileName := "root/test_write_at"
	err := ioutil.WriteFile(fileName, []byte("123456789"), 0644)
	CheckError(err, t)
	f, err := os.OpenFile(fileName, os.O_RDWR, 0644)
	CheckError(err, t)

	writeData := []byte("abc")
	n, err = f.WriteAt(writeData, 2)
	CheckError(err, t)
	err = f.Sync()
	CheckError(err, t)

	out, err := exec.Command("cat", fileName).Output()
	t.Log("WriteAt", n, err, string(out))
	CheckError(err, t)

	if n != len(writeData) || string(out) != "12abc6789" {
		t.FailNow()
	}
}

func TestWriteAtExceed(t *testing.T) {
	var n int
	fileName := "root/test_write_at_exceed"
	err := ioutil.WriteFile(fileName, []byte("123456789"), 0644)
	CheckError(err, t)
	f, err := os.OpenFile(fileName, os.O_RDWR, 0644)
	CheckError(err, t)

	writeData := []byte("abcdef")
	n, err = f.WriteAt(writeData, 7)
	CheckError(err, t)
	err = f.Sync()
	CheckError(err, t)

	out, err := exec.Command("cat", fileName).Output()
	t.Log("WriteAtExceed", n, err, string(out))
	CheckError(err, t)

	if n != len(writeData) || string(out) != "1234567abcdef" {
		t.FailNow()
	}
}
