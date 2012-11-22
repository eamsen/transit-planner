# Copyright 2011: Eugen Sawin, Philip Stahl, Jonas Sternisko
SRCDIR = src
TSTDIR = test
OUTDIR = build
OBJDIR = build/obj
CXX = g++ -Wall -g -std=c++0x -O3 -fopenmp #-DNDEBUG
MAIN_BINARIES = $(addprefix $(OUTDIR)/, $(notdir $(basename $(wildcard $(SRCDIR)/*Main.cpp))))
TEST_BINARIES = $(addprefix $(OUTDIR)/, $(notdir $(basename $(wildcard $(TSTDIR)/*Test.cpp))))
HEADER = $(wildcard $(SRCDIR)/*.h)
OBJECTS = $(addprefix $(OBJDIR)/, $(notdir $(addsuffix .o, $(basename $(filter-out %Main.cpp, $(wildcard $(SRCDIR)/*.cpp))))))
TEST_OBJECTS = $(addprefix $(OBJDIR)/, $(notdir $(addsuffix .o, $(basename $(filter-out %Main.cpp, $(wildcard $(SRCDIR)/*.cpp)) $(filter-out %Test.cpp,$(wildcard $(TSTDIR)/*.cpp))))))
MAIN_LIBS = -lboost_filesystem-mt -lboost_serialization -lboost_program_options -lboost_system -lboost_date_time -lboost_thread-mt -lrt
TEST_LIBS = -lgtest -lgtest_main $(MAIN_LIBS)
# Flags for the linker.
#LDFLAGS += --as-needed
# Additional search paths for non-system libraries
#LIB_DIR = -Llib/kdtree

.SECONDARY:
.PHONY: all optcompile test checkstyle clean

all: compile test checkstyle

compile: $(MAIN_BINARIES)

opt: CXX = g++ -Wall -std=c++0x -O3 -mtune=native -fopenmp
opt: $(MAIN_BINARIES)

test: $(TEST_BINARIES)
	rm -f log/test.log
	for T in $(TEST_BINARIES); do ./$$T; done
	rm -f data/tmp/*test*.serialized

checkstyle:
	python cpplint.py $(TSTDIR)/*.h $(TSTDIR)/*.cpp $(SRCDIR)/*.h $(SRCDIR)/*.cpp

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(MAIN_BINARIES)
	rm -f $(TEST_BINARIES)
	rm -f *.class
	rm -f *Test.TMP.*
	rm -f core
	rm -f cc-sizes.txt

$(OUTDIR)/%Main: $(OBJDIR)/%Main.o $(OBJECTS)
	$(CXX) -o $@ $^ $(LIB_DIR) $(MAIN_LIBS)

$(OUTDIR)/%Test: $(OBJDIR)/%Test.o $(TEST_OBJECTS)
	$(CXX) -o $@ $^ $(LIB_DIR) $(TEST_LIBS)

$(OBJDIR)/%Test.o: $(TSTDIR)/%Test.cpp $(HEADER)
	$(CXX) -o $(OBJDIR)/$(@F) -c $<

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADER)
	$(CXX) -o $(OBJDIR)/$(@F) -c $<
