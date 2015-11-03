.PHONY: test default clean build run

default: clean build run

clean:
	-rm -rf ../server/root/*
	-rm -f cache/*
	> client.log
	-killall clientbazil
	-sudo fusermount -u root
	-sudo fusermount -u rootlzy
	go clean

build:
	-mkdir cache
	go build

run:
	./clientbazil -m root > client.log 2>&1 &
	-go test -v
	tail -f client.log

test:
	./measure.sh
