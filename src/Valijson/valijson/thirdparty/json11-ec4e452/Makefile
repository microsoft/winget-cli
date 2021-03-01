# Environment variable to enable or disable code which demonstrates the behavior change
# in Xcode 7 / Clang 3.7, introduced by DR1467 and described here:
# https://llvm.org/bugs/show_bug.cgi?id=23812
# Defaults to on in order to act as a warning to anyone who's unaware of the issue.
ifneq ($(JSON11_ENABLE_DR1467_CANARY),)
CANARY_ARGS = -DJSON11_ENABLE_DR1467_CANARY=$(JSON11_ENABLE_DR1467_CANARY)
endif

test: json11.cpp json11.hpp test.cpp
	$(CXX) $(CANARY_ARGS) -O -std=c++11 json11.cpp test.cpp -o test -fno-rtti -fno-exceptions

clean:
	if [ -e test ]; then rm test; fi

.PHONY: clean
