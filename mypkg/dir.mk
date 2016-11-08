IMPORT_CPPFLAGS += $(patsubst %,-I%/include,$(IMPORT_TREES))
IMPORT_CPPFLAGS += $(patsubst %,-I%/pkg/pkg4c,$(IMPORT_TREES))

LIB_NAME := pkg
LIB_VERSION  := ""
LIB_OBJS := CMyPkg.o  util.o

all:: mkshared

vers  := _ ""
major := ""

namespec := $(LIB_NAME) $(vers)

LIB_IMPORTS  := ../pkg4c/libpkg4c.a

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
	@($(MV) $@ $(TOP)/lib)

clean::
	$(RM) static/*.o shared/*.o *.d *.output
