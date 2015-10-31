package main

import (
	"io/ioutil"
	"os/exec"
	"testing"
)

func CheckError(err error, t *testing.T) {
	if err != nil {
		t.Log(err)
		t.FailNow()
	}
}

func TestFileCRUD(t *testing.T) {
	//	out, err = exec.Command("ls", "-l").Output()
	//	t.Log(string(out))
	out, err := exec.Command("rm", "-rf", "root/*").Output()
	out, err = exec.Command("touch", "root/a").Output()
	CheckError(err, t)
	err = ioutil.WriteFile("root/a", []byte("FileCRUD seems OK"), 0644)
	CheckError(err, t)
	out, err = exec.Command("cat", "root/a").Output()
	CheckError(err, t)
	t.Log(string(out))
}

func TestFolderCRUD(t *testing.T) {
	out, err := exec.Command("rm", "-rf", "root/*").Output()
	CheckError(err, t)
	out, err = exec.Command("mkdir", "-p", "root/b/b").Output()
	CheckError(err, t)
	out, err = exec.Command("ls", "-l", "root/b/b").Output()
	CheckError(err, t)
	t.Log(string(out))
}
