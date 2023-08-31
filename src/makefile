all:
	gcc -c servant.c dataOperations.c dateEstateLinkedList.c estateLinkedList.c typeLinkedList.c directoryManager.c \
		socketManager.c

	gcc servant.o dataOperations.o dateEstateLinkedList.o estateLinkedList.o typeLinkedList.o directoryManager.o \
	socketManager.o -lrt -pthread -lpthread -Wall -lm -o servant

	gcc -c server.c socketManager.c requestsQueue.c
	gcc server.o socketManager.o requestsQueue.o -lrt -pthread -lpthread -Wall -lm -o server

	gcc -c client.c socketManager.c requestsQueue.c
	gcc client.o socketManager.o requestsQueue.o -lrt -pthread -lpthread -Wall -lm -o client

clean:
	rm *.o servant server client
