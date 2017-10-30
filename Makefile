PROGRAM_NAME = scheduler
CC = g++
SRCDIR = src
BUILDDIR = build
# PROGRAM_NAME = ??? <- pierwsza linia
TARGET = bin/$(PROGRAM_NAME)
CPPEXT = cpp
CFLAGS = -g -c -std=c++11
LIB =

SOURCES = $(shell find $(SRCDIR) -type f -name *.$(CPPEXT))
HEADERS = $(shell find $(SRCDIR) -type f -name *.h)

OBJECTS = $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(CPPEXT)=.o))
SHARED_OBJECTS = $(patsubst $(SRCDIR)/%,./%,$(SOURCES:.$(CPPEXT)=.o))

$(TARGET): $(OBJECTS)
		@echo " Linking..."
		@echo ">>>>>>>>>>>>>>>> $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(CPPEXT)
		@mkdir -p $(BUILDDIR)
		@echo "CPP>>>>>>>>>>>>>>>>  $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $^


clean:
		@echo " Cleaning...";
		@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

shared_library:
		$(CC)  $(CFLAGS) -fPIC $(HEADERS) $(SOURCES) $(LIB)
		$(CC) -shared -o bin/lib$(PROGRAM_NAME).so $(SHARED_OBJECTS) $(LIBS)
		sudo cp bin/lib$(PROGRAM_NAME).so /usr/local/lib
		sudo chmod 755 /usr/local/lib/lib$(PROGRAM_NAME).so
		sudo ldconfig
		sudo mkdir -p /usr/local/include/$(PROGRAM_NAME)
		sudo cp src/*.h /usr/local/include/$(PROGRAM_NAME)/
		rm *.o
		rm src/*.gch
