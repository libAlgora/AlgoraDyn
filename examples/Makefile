CC := g++

TARGETS := dynamic_graph
ACORE := ../../AlgoraCore
ADYN := ..

.PHONY: all clean

all: $(TARGETS)

clean:
	-	rm -f $(TARGETS)

% : %.cpp
	$(CC) -O3 -DNDEBUG -std=c++17 -Wall -o $@ -I$(ACORE)/src -I$(ADYN)/src/ $^ -L$(ACORE)/build/Release/ -lAlgoraDyn -L$(ADYN)/build/Release/ -lAlgoraCore

# Or, for more checks and debugging:
#% : %.cpp
#	$(CC) -O0 -std=c++17 -Wall -o $@ -I$(ACORE)/src -I$(ADYN)/src/ $^ -L$(ACORE)/build/Release/ -lAlgoraDyn -L$(ADYN)/build/Release/ -lAlgoraCore
