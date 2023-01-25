all : wav2c
clean:
	rm -rf wav2c
	rm -rf sound.c
	rm -rf sound.m
wav2c: main.c
	gcc main.c -o wav2c
