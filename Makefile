all : wav2c
clean:
	rm -rf wav2c
wav2c: main.c
	gcc main.c -o wav2c
