IMPORT_CPPFLAGS += $(patsubst %,-I%/include,$(IMPORT_TREES))
IMPORT_CPPFLAGS += $(patsubst %,-I%/pkg/mypkg,$(IMPORT_TREES))

ifdef Win32Platform

LIBS = 
TARGET = test.exe
OBJ = CMyPkg.o main.o

else

LIBS = -lpkg 
TARGET = test
OBJ = main.o 

endif

all:: $(TARGET)

$(TARGET): $(OBJ)
	@(libs="$(LIBS)"; $(CXXExecutable))
