all:
	cd player \
	make

clean:
	cd player    \
	make clean   \
	cd ../server \
	make clean

run player:
	cd player \
	make run

run server:
	cd player \
	make run