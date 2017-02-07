# Link to Itensor lib

ITENSOR_DIR=/home/urza/Software/ITensor
include $(ITENSOR_DIR)/this_dir.mk
include $(ITENSOR_DIR)/options.mk

TENSOR_HEADERS=$(ITENSOR_DIR)/itensor/core.h

# 3. 'main' function is in a file called 'get-env.cc', then
#    set APP to 'get-env'. Running 'make' will compile the app.
#    Running 'make debug' will make a program called 'get-env-g'
#    which includes debugging symbols and can be used in gdb (Gnu debugger);
APP=get-cluster-env

# 4. Add any headers your program depends on here. The make program
#    will auto-detect if these headers have changed and recompile your app.
HEADERS=cluster-ev-builder.h ctm-cluster.h ctm-cluster-io.h ctm-cluster-global.h su2.h json.hpp

# 5. For any additional .cc files making up your project,
#    add their full filenames here.
CCFILES=$(APP).cc cluster-ev-builder.cc ctm-cluster.cc ctm-cluster-io.cc su2.cc

#Mappings --------------
# see https://www.gnu.org/software/make/manual/html_node/Text-Functions.html
OBJECTS=$(patsubst %.cc,%.o, $(CCFILES))
GOBJECTS=$(patsubst %,.debug_objs/%, $(OBJECTS))

#Rules ------------------
# see https://www.gnu.org/software/make/manual/make.html#Pattern-Intro
# see https://www.gnu.org/software/make/manual/make.html#Automatic-Variables
%.o: %.cc $(HEADERS) $(TENSOR_HEADERS)
	$(CCCOM) -c $(CCFLAGS) -o $@ $<

.debug_objs/%.o: %.cc $(HEADERS) $(TENSOR_HEADERS)
	$(CCCOM) -c $(CCGFLAGS) -o $@ $<

#Targets -----------------

build: clean $(APP)
debug: $(APP)-g

$(APP): $(OBJECTS) $(ITENSOR_LIBS)
	$(CCCOM) $(CCFLAGS) $(OBJECTS) -o $(APP).x $(LIBFLAGS)

$(APP)-g: mkdebugdir $(GOBJECTS) $(ITENSOR_GLIBS)
	$(CCCOM) $(CCGFLAGS) $(GOBJECTS) -o $(APP)-g.x $(LIBGFLAGS)

clean:
	rm -fr .debug_objs *.o $(APP).x $(APP)-g.x

mkdebugdir:
	mkdir -p .debug_objs