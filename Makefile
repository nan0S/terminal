## Hubert Obrzut, lista nr 3 ##

terminal:
	gcc terminal.c -o terminal -lreadline -D _GNU_SOURCE
clean:
	rm -f terminal	
