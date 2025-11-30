PYTHON      := python3
CXX         := g++
CXXFLAGS    := -O3 -Wall -Wextra -std=c++17 -fPIC
INCLUDES    := -Iinclude
PYTHON      := python3
CXX         := g++
CXXFLAGS    := -O3 -Wall -Wextra -std=c++17 -fPIC
INCLUDES    := -Iinclude
PYBIND11_INCLUDES := $(shell $(PYTHON) -m pybind11 --includes)
EXT_SUFFIX  := $(shell $(PYTHON) -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")

# ✅ THIS IS THE IMPORTANT FIX FOR macOS + Python 3.12
PYTHON_LDFLAGS := -undefined dynamic_lookup

SRC = \
    src/Announcement.cpp \
    src/BGP.cpp \
    src/ROV.cpp \
    src/ASGraph.cpp \
    src/bindings.cpp

OBJ = $(SRC:.cpp=.o)

TARGET = bgp_sim$(EXT_SUFFIX)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -shared $(OBJ) $(PYTHON_LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(PYBIND11_INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

