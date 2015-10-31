default:
	-rm -rf root/*
	-rm -rf root/.*
	-sudo fusermount -u root
	go clean
	go build
	./clientbazil root > client.log 2>&1 &
	go test -v
	tail -f client.log
