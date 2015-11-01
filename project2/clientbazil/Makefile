default: clean
	-mkdir cache
	go build
	./clientbazil root > client.log 2>&1 &
	go test -v
	tail -f client.log

clean:
	-rm -rf root/*
	-rm -rf root/.*
	-killall clientbazil
	-sudo fusermount -u root
	go clean
