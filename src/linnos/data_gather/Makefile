# Makefile for running insert_mod.sh 10 times

# Number of times to run the script
NUM_RUNS := 1

# Default target
.PHONY: all
all: run_script

# Rule to run the script multiple times
.PHONY: run_script
run_script:
	@for i in $$(seq 1 $(NUM_RUNS)); do \
		./run_benchmark.sh; \
    done

# Clean rule
.PHONY: clean
clean:
    # Add clean commands if needed

