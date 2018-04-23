#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int linumer = 0;
struct history_entry hist_str[HIST_MAX];

static int print_index = 0;

void print_history() {
    /* This function should print history entries */
    int i;
	if (linumer < HIST_MAX) {
		for (i = 0; i < linumer; i ++) {

			printf("[%lu|%.2f] %s\n", hist_str[i].cmd_id, hist_str[i].run_time, hist_str[i].cmd_name);
		}
		
	} else {

		for (i = 0; i < HIST_MAX; i ++) {
			int index = (print_index + i) % HIST_MAX;
			printf("[%lu|%.2f] %s\n", hist_str[index].cmd_id, hist_str[index].run_time, hist_str[index].cmd_name);
		}
	}
    	
}

int add_history(char *cmd, double runtime) {

	if (linumer >= HIST_MAX) {
		print_index = (print_index + 1) % HIST_MAX;
	}

	hist_str[linumer%HIST_MAX].cmd_id = linumer;
	strcpy(hist_str[linumer%HIST_MAX].cmd_name, cmd);
	hist_str[linumer%HIST_MAX].run_time = runtime;

	linumer++;

	return 1;
}

int search_cmd(char* cmd) {

	int i = linumer - 1;
	while(i > -1 && i >= linumer - HIST_MAX) {

		if (strcmp(hist_str[i % HIST_MAX].cmd_name, cmd) == 0) {

			return i;
		}
		i--;
	}

	return -1;
}