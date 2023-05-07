CFLAGS = -Wall -Werror -g
CC = gcc $(CFLAGS)
SHELL = /bin/bash
CWD = $(shell pwd | sed 's/.*\///g')
AN = proj1

minitar: minitar_main.c file_list.o minitar.o
	$(CC) -o minitar minitar_main.c file_list.o minitar.o -lm

file_list.o: file_list.h file_list.c
	$(CC) -c file_list.c

minitar.o: minitar.h minitar.c
	$(CC) -c minitar.c

test-setup:
	@chmod u+x testius

ifdef testnum
test: minitar test-setup
	./testius test_cases/tests.json -v -n "$(testnum)"
else
test: minitar test-setup
	./testius test_cases/tests.json
endif

clean:
	rm -f *.o minitar

clean-tests:
	rm -rf test_results test_files test.tar

zip: clean clean-tests
	rm -f proj1-code.zip
	cd .. && zip "$(CWD)/$(AN)-code.zip" -r "$(CWD)" -x "$(CWD)/test_cases/*" "$(CWD)/testius"
	@echo Zip created in $(AN)-code.zip
	@if (( $$(stat -c '%s' $(AN)-code.zip) > 10*(2**20) )); then echo "WARNING: $(AN)-code.zip seems REALLY big, check there are no abnormally large test files"; du -h $(AN)-code.zip; fi
	@if (( $$(unzip -t $(AN)-code.zip | wc -l) > 256 )); then echo "WARNING: $(AN)-code.zip has 256 or more files in it which may cause submission problems"; fi
