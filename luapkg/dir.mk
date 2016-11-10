IMPORT_CPPFLAGS += $(patsubst %,-I%/include,$(IMPORT_TREES))
IMPORT_CPPFLAGS += $(patsubst %,-I%/pkg/mypkg,$(IMPORT_TREES))
IMPORT_LIBRARY_FLAGS += $(patsubst %,-L%/pkg/luapkg,$(IMPORT_TREES))

LIBS = -lpkg -llua5.2
TARGET = test
OBJ = main.o 

all:: $(TARGET)

$(TARGET): $(OBJ)
	@(libs="$(LIBS)"; $(CXXExecutable))
