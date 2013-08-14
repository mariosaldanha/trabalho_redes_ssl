#Makefile

#Compilador
CC=g++

# Objects
OBJETOS=http.o webcrawler.o

# Compilador flags - Warning, debug, boost,  SSL
CPPFLAGS=-std=c++0x -Wall -g -lm -lboost_regex -lssl -lcrypto

EXECUTAVEL=testeme
BASE_FILENAME=trabalho_redes_-_webcrawler

all: $(OBJETOS)
	$(CC) $(OBJETOS) $(CPPFLAGS) -o $(EXECUTAVEL)
	rm -rf $(OBJETOS)

webcrawler: http.h webcrawler.cpp
	$(CC) $(CPPFLAGS) -c webcrawler.cpp

http: http.h http.cpp
	$(CC) $(CPPFLAGS) -c http.cpp

clean:
	rm -rf $(EXECUTAVEL)

package:
	@echo "Empacotando: "
ifneq ($(wildcard *$(BASE_FILENAME)*),)
	rename 's/tar.gz/$(REVISION).tar.gz' $(wildcard *$(BASE_FILENAME)*) 
	#@echo "Arquivo Existe"
endif
ifeq ($(wildcard $(BASE_FILENAME).tar.gz),)
	mkdir $(BASE_FILENAME)
	cp -R README.txt *.cpp *.h makefile $(BASE_FILENAME)/
	tar -zcvf $(BASE_FILENAME).tar.gz $(BASE_FILENAME)/
	#zip -r $(BASE_FILENAME).zip $(BASE_FILENAME)/
	rm -R $(BASE_FILENAME)/
endif
