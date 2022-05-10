.PHONY: tests real_tests clean mdp_clean lines

tests:
	@mdp README.md
	@$(MAKE) --no-print-directory real_tests

include $(wildcard deps/*.dep)

CXXFLAGS += -g -Wall -std=c++17

APP := server

build/%.o: %.cpp
	@echo C++ $@
	@mkdir -p build deps
	@clang++ $(CXXFLAGS) -c $(notdir $(@:.o=.cpp)) -o $@ -MMD -MF deps/$(notdir $(@:.o=.dep))

CPPs := server.cpp socket.cpp
OBJs := $(addprefix build/,$(CPPs:.cpp=.o))

$(APP): $(OBJs)
	@echo LINK $@
	@clang++ $(CXXFLAGS) -o $@ $(OBJs)

real_tests: $(APP)
	@echo NO TESTS YET	

clean:
	@echo CLEAN
	@rm -Rf $(APP)

mdp_clean: clean
	@echo MDP CLEAN
	@rm *.cpp *.h

lines:
	@echo LINES
	@cat $(wildcard *.cpp) $(wildcard *.h) Makefile | wc -l
