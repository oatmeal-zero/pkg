IMPORT_CPPFLAGS += $(patsubst %,-I%/include,$(IMPORT_TREES))
IMPORT_CPPFLAGS += $(patsubst %,-I%/3rd/jsoncpp,$(IMPORT_TREES))
IMPORT_CPPFLAGS += $(patsubst %,-I%/tools/mypkg,$(IMPORT_TREES))
IMPORT_LIBRARY_FLAGS += $(patsubst %,-L%/3rd/jsoncpp,$(IMPORT_TREES))

ifdef Win32Platform

LIBS = 
TARGET = test.exe
OBJ = CMyPkg.o main.o

else

LIBS = -lpkg -ljson
TARGET = test
OBJ = main.o 

endif

all:: $(TARGET)

$(TARGET): $(OBJ)
	@(libs="$(LIBS)"; $(CXXExecutable))
