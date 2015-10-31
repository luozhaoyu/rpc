default:
	-rm -rf root/*
	-sudo fusermount -u root
	go clean
	go build
	./clientbazil root
