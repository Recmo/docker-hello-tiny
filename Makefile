all: serve

run: all container
	time docker run -p 8080:8080 -ti minimal-docker

container: serve
	docker build . -t minimal-docker

serve: serve.o EventLoop.o EventHandler.o Signals.o Timer.o fail.o Listener.o Socket.o Thread.o HttpRequestParser.o Response.o

%.o: src/%.cpp
	clang++ -std=c++14 -march=native -Os -fno-exceptions -fno-asynchronous-unwind-tables -fno-rtti -c $< -o $@

%: %.o
	ld.gold --plugin /usr/lib/LLVMgold.so -s $^ -o $@
	strip -s -R .comment -R .note.gnu.gold-version $@
