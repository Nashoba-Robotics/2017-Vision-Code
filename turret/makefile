camera: camera.cpp
	rm -rf build
	mkdir build
	g++ -ggdb `pkg-config opencv --cflags --libs` -o build/camera camera.cpp tcp_client.cpp `pkg-config --libs opencv` -Wall -Werror -Wextra

run: 
	./build/camera

clean:
	rm -rf build
