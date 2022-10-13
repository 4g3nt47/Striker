#----------------------------------
# The Makefile for building agents.
#----------------------------------

cc = gcc -Os -Wall
ifeq ($(debug), 1)
  cc += -ggdb3
else
	cc += -s
endif
cc_obj = $(cc) -c

ifeq ($(os), win)
bin/stub.exe: bin/stub.o bin/striker_utils.o bin/cJSON.o bin/builder.exe
	$(cc) bin/stub.o bin/striker_utils.o bin/cJSON.o -o bin/stub.exe
else
bin/stub: bin/stub.o bin/striker_utils.o bin/cJSON.o bin/builder
	$(cc) bin/stub.o bin/striker_utils.o bin/cJSON.o -lcurl -lpthread -o bin/stub
endif

bin/stub.o: src/striker.c src/striker.h bin/striker_utils.o
	$(cc_obj) src/striker.c -o bin/stub.o

bin/striker_utils.o: src/striker_utils.c src/striker_utils.h
	$(cc_obj) src/striker_utils.c -o bin/striker_utils.o

bin/cJSON.o: src/cJSON.c src/cJSON.h
	$(cc_obj) src/cJSON.c -o bin/cJSON.o

ifeq ($(os), win)
bin/builder.exe: src/builder.c bin/obfuscator.o
	$(cc) src/builder.c bin/obfuscator.o -o bin/builder.exe
else
bin/builder: src/builder.c bin/obfuscator.o
	$(cc) src/builder.c bin/obfuscator.o -o bin/builder
endif

bin/obfuscator.o: src/obfuscator.c src/obfuscator.h
	$(cc_obj) src/obfuscator.c -o bin/obfuscator.o

ifeq ($(os), win)
clean:
	del /F /Q bin
else
clean:
	rm bin/*
endif
