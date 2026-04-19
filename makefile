CXX := g++
CXXFLAGS_CXX20 := -std=c++20 -O2 -I./xbox
CXXFLAGS_CXX14 := -std=c++14 -O2 -I./xbox
LDFLAGS_URING := -luring

all: test_echo test_operator test_json

test_echo: test/test_echo.cpp
	$(CXX) $(CXXFLAGS_CXX20) $< -o $@ $(LDFLAGS_URING)

test_operator: test/test_operator.cpp
	$(CXX) $(CXXFLAGS_CXX20) $< -o $@ $(LDFLAGS_URING)

test_json: test/test_json.cpp
	$(CXX) $(CXXFLAGS_CXX14) $< -o $@

clean:
	rm -f test_echo test_operator test_json
