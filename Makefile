all: demo

demo:
	gcc -static demo.c -o demo

clean:
	rm -f demo

