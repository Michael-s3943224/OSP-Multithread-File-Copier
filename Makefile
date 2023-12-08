
##
 # startup code provided by Paul Miller for COSC1114 - Operating Systems
 # Principles
 ##

# provide make targets here to build the two programs 

CXX := g++
CXXFLAGS := -Wall -Werror -std=c++20 -g -O

STCOPYDIR := ./copier_files
MTCOPYDIR := ./mtcopier_files
SSTCOPYDIR := ./sane_copier_files
BSTCOPYDIR := ./better_copier_files
BMTCOPYDIR := ./better_mtcopier_files
MTCOPY2DIR := ./mtcopier_files2

STOBJS := $(patsubst %.cpp,%.o,$(wildcard $(STCOPYDIR)/*.cpp))
MTOBJS := $(patsubst %.cpp,%.o,$(wildcard $(MTCOPYDIR)/*.cpp))
SSTOBJS := $(patsubst %.cpp,%.o,$(wildcard $(SSTCOPYDIR)/*.cpp))
BSTOBJS := $(patsubst %.cpp,%.o,$(wildcard $(BSTCOPYDIR)/*.cpp))
BMTOBJS := $(patsubst %.cpp,%.o,$(wildcard $(BMTCOPYDIR)/*.cpp))
MT2OBJS := $(patsubst %.cpp,%.o,$(wildcard $(MTCOPY2DIR)/*.cpp))

.default: all

all: copier mtcopier scopier bmtcopier bcopier mtcopier2

copier: $(STOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(STCOPYDIR)/%.o: $(STCOPYDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

scopier: $(SSTOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SSTCOPYDIR)/%.o: $(SSTCOPYDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

mtcopier: $(MTOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

$(MTCOPYDIR)/%.o: $(MTCOPYDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -lpthread

bcopier: $(BSTOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BSTCOPYDIR)/%.o: $(BSTCOPYDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

bmtcopier: $(BMTOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

$(BMTCOPYDIR)/%.o: $(BMTCOPYDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -lpthread

mtcopier2: $(MT2OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

$(MTCOPY2DIR)/%.o: $(MTCOPY2DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^ -lpthread

clean:
	rm -rf copier scopier mtcopier bcopier bmtcopier mtcopier2 $(STCOPYDIR)/*.o $(MTCOPYDIR)/*.o $(SSTCOPYDIR)/*.o $(BSTCOPYDIR)/*.o $(BMTCOPYDIR)/*.o $(MTCOPY2DIR)/*.o *.dSYM
