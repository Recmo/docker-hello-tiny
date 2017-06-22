all: serve

run: all
	time docker run -ti minimal-docker

container: serve
	docker build . -t minimal-docker

serve: serve.o

%.o: %.cpp
	clang++ -std=c++14 -Os -fno-asynchronous-unwind-tables -c $< -o $@

%: %.o
	ld -s $< -o $@
	strip -R .comment $@
