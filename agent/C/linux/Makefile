bin/stub: bin/stub.o bin/striker_utils.o bin/cJSON.o bin/builder
	gcc -Os -ggdb3 -Wall bin/stub.o bin/striker_utils.o bin/cJSON.o -lcurl -lpthread -o bin/stub
# 	gcc -Os -s -Wall bin/stub.o bin/striker_utils.o bin/cJSON.o -lcurl -lpthread -o bin/stub

bin/stub.o: src/striker.c src/striker.h bin/striker_utils.o
	gcc -Os -ggdb3 -c -Wall src/striker.c -o bin/stub.o
# 	gcc -Os -c -Wall src/striker.c -o bin/stub.o

bin/striker_utils.o: src/striker_utils.c src/striker_utils.h
	gcc -Os -ggdb3 -c -Wall src/striker_utils.c -o bin/striker_utils.o
# 	gcc -Os -c -Wall src/striker_utils.c -o bin/striker_utils.o

bin/cJSON.o: src/cJSON.c src/cJSON.h
	gcc -Os -ggdb3 -c -Wall src/cJSON.c -o bin/cJSON.o
# 	gcc -Os -c -Wall src/cJSON.c -o bin/cJSON.o

bin/builder: src/builder.c bin/obfuscator.o
	gcc -Os -ggdb3 -Wall src/builder.c bin/obfuscator.o -o bin/builder
# 	gcc -Os -s -Wall src/builder.c bin/obfuscator.o -o bin/builder

bin/obfuscator.o: src/obfuscator.c src/obfuscator.h
	gcc -Os -ggdb3 -c -Wall src/obfuscator.c -o bin/obfuscator.o
# 	gcc -Os -c -Wall src/obfuscator.c -o bin/obfuscator.o

clean:
	rm bin/*
