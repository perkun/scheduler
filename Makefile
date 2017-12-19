CC=g++
SRC = src
BUILD = build
BIN = bin

CFLAGS = -g -c -std=c++11 -Wall
LFLAGS = -g -Wall -std=c++11
LIBS = -lpqxx -lncurses -lrt



OBJS = $(BUILD)/Job.o $(BUILD)/JobList.o $(BUILD)/MessageQueue.o $(BUILD)/scheduler.o $(BUILD)/Options.o $(BUILD)/Computer.o


#all: $(BUILD)/JobList.o
all: $(BIN)/scheduler #$(BIN)/mqSend $(BIN)/crankshaft $(BIN)/msqtest


#################### EXECUTABLES ####################
$(BIN)/scheduler: $(OBJS)
		$(CC) $(LFLAGS) $(OBJS) -o $(BIN)/scheduler $(LIBS)

$(BIN)/crankshaft: $(BUILD)/crankshaft.o $(BUILD)/MessageQueue.o
		$(CC) $(LFLAGS) $(BUILD)/crankshaft.o $(BUILD)/MessageQueue.o  -o $(BIN)/crankshaft

$(BIN)/msqtest: $(BUILD)/msqtest.o $(BUILD)/MessageQueue.o
		$(CC) $(LFLAGS) $(BUILD)/msqtest.o $(BUILD)/MessageQueue.o  -o $(BIN)/msqtest -lrt

$(BIN)/mqSend:
		gcc src/mqSend.c -o bin/mqSend -lrt



################### OBJECT FILES ###################
$(BUILD)/crankshaft.o: $(SRC)/crankshaft.cpp $(SRC)/MessageQueue.h
		@mkdir -p $(BUILD)
		$(CC) $(CFLAGS) $(SRC)/crankshaft.cpp -o $(BUILD)/crankshaft.o

$(BUILD)/msqtest.o: $(SRC)/msqtest.cpp $(SRC)/MessageQueue.h
		@mkdir -p $(BUILD)
		$(CC) $(CFLAGS) $(SRC)/msqtest.cpp -o $(BUILD)/msqtest.o

$(BUILD)/scheduler.o: $(SRC)/scheduler.cpp $(SRC)/MessageQueue.h $(SRC)/Job.h $(SRC)/JobList.h
		@mkdir -p $(BUILD)
		$(CC) $(CFLAGS) $(SRC)/scheduler.cpp -o $(BUILD)/scheduler.o


$(BUILD)/MessageQueue.o: $(SRC)/MessageQueue.cpp $(SRC)/MessageQueue.h
		@mkdir -p $(BUILD)
		$(CC) $(CFLAGS) $(SRC)/MessageQueue.cpp -o $(BUILD)/MessageQueue.o

$(BUILD)/Job.o: $(SRC)/Job.cpp $(SRC)/Job.h
		@mkdir -p $(BUILD)
		$(CC) $(CFLAGS) $(SRC)/Job.cpp -o $(BUILD)/Job.o

$(BUILD)/JobList.o: $(SRC)/JobList.cpp $(SRC)/JobList.h $(SRC)/Job.h
		@mkdir -p $(BUILD)
		$(CC) $(CFLAGS) $(SRC)/JobList.cpp -o $(BUILD)/JobList.o

$(BUILD)/Computer.o: $(SRC)/Computer.cpp $(SRC)/Computer.h
		@mkdir -p $(BUILD)
		$(CC) $(CFLAGS) $(SRC)/Computer.cpp -o $(BUILD)/Computer.o

$(BUILD)/Options.o: $(SRC)/Options.cpp $(SRC)/Options.h
		@mkdir -p $(BUILD)
		$(CC) $(CFLAGS) $(SRC)/Options.cpp -o $(BUILD)/Options.o

clean:
		rm -r $(BUILD) $(BIN)/scheduler $(BIN)/mqSend $(BIN)/crankshaft
