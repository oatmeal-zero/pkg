IMPORT_CPPFLAGS += $(patsubst %,-I%/include,$(IMPORT_TREES))
IMPORT_CPPFLAGS += -Wall

LIB_NAME := pkg4c
LIB_VERSION  := ""
LIB_OBJS := pkg4c.o rbtree.o sds.o zmalloc.o cJSON.o json_pkg.o

all:: mkstatic

vers  := _ ""
major := ""

namespec := $(LIB_NAME) $(vers)

LIB_IMPORTS  := 

##############################################################################
# Build Static library
##############################################################################

staticlib := lib$(LIB_NAME)$(major).a

mkstatic::
	@(dir=static; $(CreateDir))

mkstatic:: $(staticlib)
$(staticlib): $(patsubst %, static/%, $(LIB_OBJS))
	@$(StaticLinkLibrary)

##############################################################################
# Build Shared library
##############################################################################

shlib := lib$(LIB_NAME)$(major).so

imps  := $(LIB_IMPORTS)

mkshared::
	@(dir=shared; $(CreateDir))

mkshared:: $(shlib)
$(shlib): $(patsubst %, shared/%, $(LIB_OBJS))
	@(namespec="$(namespec)" extralibs="$(imps) $(extralibs)" nodeffile=1; \
	$(MakeCXXSharedLibrary))

install::
	@($(CP) $(shlib) $(TOP)/lib)

clean::
	$(RM) static/*.o shared/*.o *.d *.output
