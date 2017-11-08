CC=g++
SRC = src
BUILD = build
BIN = bin

CFLAGS = -g -c -std=c++11 -Wall
LFLAGS = -g -Wall -std=c++11




OBJS = $(BUILD)/Job.o $(BUILD)/JobList.o $(BUILD)/MessageQueue.o $(BUILD)/scheduler.o




all: $(BIN)/scheduler $(BIN)/mqSend $(BIN)/crankshaft

# EXECUTABLES
$(BIN)/scheduler: $(OBJS)
		$(CC) $(LFLAGS) $(OBJS) -o $(BIN)/scheduler

$(BIN)/crankshaft: $(BUILD)/crankshaft.o $(BUILD)/MessageQueue.o
		$(CC) $(LFLAGS) $(BUILD)/crankshaft.o $(BUILD)/MessageQueue.o  -o $(BIN)/crankshaft

$(BIN)/mqSend:
		gcc mqSend.c -o mqSend

# OBJECT FILES

$(BUILD)/crankshaft.o: $(SRC)/crankshaft.cpp $(SRC)/MessageQueue.h
		@mkdir -p $(BUILD)
		$(CC) $(CFLAGS) $(SRC)/crankshaft.cpp -o $(BUILD)/crankshaft.o


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

clean:
		rm -r $(BUILD) $(BIN)/scheduler
