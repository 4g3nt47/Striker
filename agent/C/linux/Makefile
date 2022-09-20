bin/striker: bin/striker.o bin/striker_utils.o bin/cJSON.o
	gcc -Os -s -Wall bin/striker.o bin/striker_utils.o bin/cJSON.o -lcurl -o bin/striker

bin/striker.o: src/striker.c src/striker.h bin/striker_utils.o
	gcc -Os -c -Wall src/striker.c -o bin/striker.o

bin/striker_utils.o: src/striker_utils.c src/striker_utils.h
	gcc -Os -c -Wall src/striker_utils.c -o bin/striker_utils.o

bin/cJSON.o: src/cJSON.c src/cJSON.h
	gcc -Os -c -Wall src/cJSON.c -o bin/cJSON.o

builder: src/builder.c src/builder.h
	gcc -Os -s -Wall src/builder.c -o bin/builder

clean:
	rm bin/*
