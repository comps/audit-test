SHELL	:= /bin/bash
run:
	@IFS=$$'\n'; \
	dirs=( $$(find /* -maxdepth 0 -type d \
		-not \( -path /proc -o -path /sys -o -path /dev -o -path /run \)) ); \
	for d in "$${dirs[@]}"; do \
		echo "scanning $$d"; \
		./relroscan.sh "$$d" >> out.log 2>> out.err; \
	done;

.PHONY: clean
clean:
	rm -f out.log out.err
