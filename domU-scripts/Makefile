all:
	gcc -o report_traff_per_interval report_traff_per_interval.c -lpcap
	gcc -o generate_loads generate_loads.c -pthread -lm
	gcc -o prep_for_generate_loads prep_for_generate_loads.c -pthread -lm

debug:
	gcc -o report_traff_per_interval report_traff_per_interval.c -lpcap -DDEBUG_SS
	gcc -o generate_loads generate_loads.c -pthread -lm -DDEBUG_SS
	gcc -o prep_for_generate_loads prep_for_generate_loads.c -pthread -lm -DDEBUG_SS

time:
	gcc -o report_traff_per_interval report_traff_per_interval.c -lpcap
	gcc -o generate_loads generate_loads.c -pthread -lm -DTIME_DEBUG_SS
	gcc -o prep_for_generate_loads prep_for_generate_loads.c -pthread -lm -DTIME_DEBUG_SS

clean:
	rm generate_loads report_traff_per_interval prep_for_generate_loads
