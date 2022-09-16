bin/striker: bin/striker.o bin/cJSON.o
		gcc -Os -s -Wall bin/striker.o bin/cJSON.o -lcurl -o bin/striker
bin/striker.o: src/striker.c src/striker.h
		gcc -Os -c -Wall src/striker.c -o bin/striker.o
bin/cJSON.o: src/cJSON.c src/cJSON.h
		gcc -Os -c src/cJSON.c -o bin/cJSON.o
