LIBS=-lpthread
OBJECT=chatserver client
all : $(OBJECT)

chatserver : main.cpp chatserver.cpp strtools.cpp
	g++ $^ -o $@ $(LIBS)

client : client.cpp strtools.cpp
	g++ $^ -o $@ $(LIBS)

clean :
	rm -f *.o $(OBJECT)
