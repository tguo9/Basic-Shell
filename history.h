#ifndef _HISTORY_H_
#define _HISTORY_H_

#define HIST_MAX 100

struct history_entry {
    unsigned long cmd_id;
    double run_time;
    char cmd_name[1000];
};

/* 
 * Input: Nothing
 * Output: Nothing
 * 
 * Purpose: Print history
 */
void print_history();

/* 
 * Input: Command and runtime
 * Output: Int 1, keep the program running
 * 
 * Purpose: Add to history
 */
int add_history(char *cmd, double runtime);

/* 
 * Input: Command
 * Output: Index
 * 
 * Purpose: Search the command from history
 */
int search_cmd(char* cmd);
extern int linumer;
extern struct history_entry hist_str[HIST_MAX];
#endif