#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#define arg(i, x) (strcmp(argv[i], x) == 0)

#define VARIABLE_NOT_SPECIFIED 	0
#define AMOUNT_NOT_SPECIFIED   	1
#define VALUE_NOT_SPECIFIED    	2
#define TIMER_NOT_SPECIFIED 	3
#define FILE_CREATION_FAIL 	4
#define FILE_OPEN_FAIL 		5
#define TIMER_INVALID           6
#define FILE_REMOVE_FAIL	7
#define TYPE_NOT_SPECIFIED      8
#define NEW_NOT_SPECIFIED       9
#define ITEM_NOT_SPECIFIED      10
#define TYPE_INVALID            11
#define PATH_SET_FAIL           12

const char* err_code[] = {
	"Error: Variable not specified.",
	"Error: Amount not specified.",
	"Error: Value not specified.",
	"Error: Timer not specified.",
	"Error: Failed to create file.",
	"Error: Failed to open file.",
	"Error: Invalid timer.",
	"Error: Failed to remove file.",
	"Error: Type not specified.",
	"Error: New name not specified.",
	"Error: Item not specified.",
	"Error: Invalid type.",
	"Error: Failed to set path."
};

void err(size_t n) {
	printf("%s\n", err_code[n]);
}

typedef struct trace {
	size_t lines;
	char** trace;
} trace;

trace read_file(FILE* file) {
	char c;
	trace res = {.lines = 0};
	if (file == NULL) {
		return (trace){.lines = 0, .trace = NULL};
	}
	while ((c = getc(file)) != EOF)
		if (c == '\n') res.lines++;
	rewind(file);
	res.trace = malloc(sizeof(char*) * res.lines);
	size_t line_len;
	size_t line_cap;
	for (size_t i = 0; i < res.lines; i++) {
		line_len = 0;
		line_cap = 64;
		res.trace[i] = malloc(sizeof(char) * line_cap);
		while((c = getc(file)) != '\n') {
			if (line_len == line_cap) {
				line_cap *= 2;
				res.trace[i] = realloc(res.trace[i], sizeof(char) * line_cap);
			}
			res.trace[i][line_len++] = c;
		}
	}
	return res;
}

char* read_line(FILE* file) {
	char c;
	size_t len = 0;
	if (file == NULL) {
		return NULL;
	}
	while ((c = getc(file)) != '\n' && c != EOF)
		len++;
	rewind(file);
	size_t i = 0;
	char* line = malloc(sizeof(char) * (len + 1));
	while ((c = getc(file)) != '\n' && c != EOF)
		line[i++] = c;
	line[i] = '\0';
	return line;
}

void print_trace(trace file) {
	for (size_t i = 0; i < file.lines; i++) {
		printf("%s\n", file.trace[i]);
	}
}
void print_trace_to_file(FILE* fileptr, trace file) {
	for (size_t i = 0; i < file.lines; i++) {
		fprintf(fileptr, "%s\n", file.trace[i]);
	}
	fclose(fileptr);
}
void free_trace(trace file) {
	for (size_t i = 0; i < file.lines; i++) {
		free(file.trace[i]);
		file.trace[i] = NULL;
	}
	free(file.trace);
	file.trace = NULL;
	file.lines = 0;
}

void trace_add_line(trace* file, const char* line) {
	file->lines++;
	file->trace = realloc(file->trace, sizeof(char*)*file->lines);
	size_t line_len = strlen(line);
	size_t line_cap = 64;
	while (line_cap < line_len) line_cap *= 2;
	file->trace[file->lines - 1] = malloc(sizeof(char) * line_cap);
	strcpy(file->trace[file->lines - 1], line);
}

void tracey_help_meta() {
	printf("tracey help meta                                prints this section of the help screen\n");
	printf("tracey path                                               output where tracey operates\n");
	printf("tracey path <PATH>                                set <PATH> where tracey will operate\n");
}

void tracey_help_timer() {
	printf("tracey help timer				prints this section of the help screen\n");
	printf("tracey start <TIMER>					 	       creates <TIMER>\n");
	printf("tracey rename timer <OLD> <NEW>                           renames <OLD> timer to <NEW>\n");
	printf("tracey time <TIMER>    		       outputs time elapsed since the start of <TIMER>\n");
	printf("tracey time <TIMER> into <VARIABLE>\n");
	printf("	    outputs time elapsed since the start of <TIMER> into <VARIABLE> in a trace\n");
	printf("tracey all timers         lists all timers and time elapsed since the creation of each\n");
	printf("tracey stop <TIMER>					               removes <TIMER>\n");
	printf("tracey copy <TIMER> <COPY>                                  copies <TIMER> into <COPY>\n");	
}

void tracey_help_trace() {
	printf("tracey help trace				prints this section of the help screen\n");
	printf("tracey set <VARIABLE> <VALUE>			 sets <VARIABLE> to <VALUE> in a trace\n");
	printf("tracey add <VARIABLE> <AMOUNT>		  increments <VARIABLE> by <AMOUNT> in a trace\n");
	printf("tracey sub <VARIABLE> <AMOUNT>	       	  decrements <VARIABLE> by <AMOUNT> in a trace\n");
	printf("tracey mul <VARIABLE> <AMOUNT>            multiplies <VARIABLE> by <AMOUNT> in a trace\n");
	printf("tracey div <VARIABLE> <AMOUNT>		     divides <VARIABLE> by <AMOUNT> in a trace\n");
	printf("tracey erase <VARIABLE>				      erases a <VARIABLE> from a trace\n");
	printf("tracey trace					           outputs last trace contents\n");
	printf("tracey trace <TRACE>    			       	      outputs <TRACE> contents\n");
	printf("tracey all traces                                 list all traces and contents of each\n");
	printf("tracey copy trace <TRACE>                               copies <TRACE> into last trace\n");
	printf("tracey copy trace <TRACE> <COPY>                            copies <TRACE> into <COPY>\n");
}

void tracey_help_item() {
	printf("tracey help item				prints this section of the help screen\n");
	printf("tracey new <ITEM> [<VARIABLE> <VALUE>]	               creates <ITEM> with <VARIABLE>s\n");
	printf("tracey rename item <OLD> <NEW>                             renames <OLD> item to <NEW>\n");
	printf("tracey add <ITEM>\n");
	printf("                 adds variables found in <ITEM> and increments <ITEM> count in a trace\n");
	printf("tracey sub <ITEM>\n");
	printf("            subtracts variables found in <ITEM> and decrements <ITEM> count in a trace\n");
	printf("tracey remove <ITEM>					                removes <ITEM>\n");
	printf("tracey item <ITEM>					       outputs <ITEM> contents\n");
	printf("tracey all items                                   list all items and contents of each\n");
	printf("tracey copy item <ITEM> <COPY>                               copies <ITEM> into <COPY>\n");
}

int main(int argc, char** argv, char** envp) {
	if (argc == 1) {
		printf("TRACEY\n");
		printf("Idiosyncratic tracker.\n");
		printf("Type \"tracey help\" for more info.\n");
		return 0;
	}
	FILE* path = fopen(".tracey/PATH", "r");
	char* pathstr = NULL;
	if (path != NULL) {
		pathstr = read_line(path);
		if (pathstr[strlen(pathstr)-1] != '/') {
			char* newpathstr = malloc(sizeof(char) * (strlen(pathstr) + 2));
			strcpy(newpathstr, pathstr);
			newpathstr[strlen(pathstr)] = '/';
			newpathstr[strlen(pathstr)+1] = '\0';
			free(pathstr);
			pathstr = newpathstr;
		}
		fclose(path);
	}
	if (arg(1, "path")) {
		if (argc == 2) {
			if (pathstr == NULL || strlen(pathstr) == 0) {
				printf(".\n");
				if (pathstr) free(pathstr);
				return 0;
			}
			printf("%s\n", pathstr);
			free(pathstr);
			return 0;
		}
		if (pathstr) free(pathstr);
		path = fopen(".tracey/PATH", "w+");
		if (path != NULL) goto dir_exists;
		const char* dir = ".tracey";
		mode_t mode = 0755;
		if (mkdir(dir, mode) == 0) {
			path = fopen(".tracey/PATH", "w+");
		dir_exists:
			struct stat s;
			if (path == NULL) {
				goto path_set_fail;
			}
			if (stat(argv[2], &s) != 0) {
				fclose(path);
				goto path_set_fail;
			}
			if (S_ISDIR(s.st_mode) == 0) {
				fclose(path);
				goto path_set_fail;
			}
			if (fprintf(path, "%s", argv[2]) < 0) {
				fclose(path);
				goto path_set_fail;	
			}
			fclose(path);
			if (argv[2][strlen(argv[2])-1] == '/')
				printf("Success: Path set to %s.\n", argv[2]);
			else 
				printf("Success: Path set to %s/.\n", argv[2]);
			return 0;
		} else {
path_set_fail:
			err(PATH_SET_FAIL);
			return 0;
		}
	}
	if (arg(1, "help")) {
		if (argc == 2) goto general_help;
		if (arg(2, "meta")) {
			printf("\n");
			tracey_help_meta();
			printf("\n");
			return 0;
		}
		if (arg(2, "timer")) {
			printf("\n");
			tracey_help_timer();
			printf("\n");
			return 0;
		}
		if (arg(2, "trace")) {
			printf("\n");
			tracey_help_trace();
			printf("\n");
			return 0;
		}
		if (arg(2, "item")) {
			printf("\n");
			tracey_help_item();
			printf("\n");
			return 0;
		}
general_help:
		printf("\n");
		printf("tracey help					               prints this help screen\n");
		printf("MOST OF THE COMMANDS DO NOT WORK YET, USE THIS SCREEN TO SEE WHAT WILL GET ADDED\n");
		printf("\n");
		tracey_help_meta();
		printf("\n");
		tracey_help_timer();
		printf("\n");
		tracey_help_trace();
		printf("\n");
		tracey_help_item();
		printf("\n");
		return 0;
	}
	if (arg(1, "start")) {
		if (argc == 2) {
			err(TIMER_NOT_SPECIFIED);
			printf("tracey start <TIMER>\n");
			return 0;
		}
		char* timer_name = malloc(strlen(argv[2]) + 10);
		sprintf(timer_name, "%s.timer", argv[2]);
		FILE* timer = fopen(timer_name, "w");
		if (timer == NULL) {
			err(FILE_CREATION_FAIL);
			free(timer_name);
			return 0;
		}
		fprintf(timer, "%lu", (unsigned long)time(NULL));
		fclose(timer);
		free(timer_name);
		return 0;
	}
	if (arg(1, "time")) {
		if (argc == 2) {
			err(TIMER_NOT_SPECIFIED);
			printf("tracey time <TIMER>\n");
			return 0;
		}
		char* timer_name = malloc(strlen(argv[2]) + 10);
		sprintf(timer_name, "%s.timer", argv[2]);
		FILE* timer = fopen(timer_name, "r");
		if (timer == NULL) {
			err(FILE_OPEN_FAIL);
			free(timer_name);
			return 0;
		}
		free(timer_name);
		char* line = read_line(timer);
		char* end;
		fclose(timer);
		unsigned long timer_time = strtoul(line, &end, 10);
		free(line);
		if (timer_time == 0) {
			err(TIMER_INVALID);
			return 0;
		}
		unsigned long now = time(NULL);
		if (timer_time > now) {
			err(TIMER_INVALID);
			return 0;
		}
		unsigned long s = now - timer_time;
		printf("%lu:%02lu:%02lu\n", s/3600, (s%3600)/60, (s%60));
		return 0;
	}
	if (arg(1, "stop")) {
		if (argc == 2) {
			err(TIMER_NOT_SPECIFIED);
			printf("tracey stop <TIMER>\n");
			return 0;
		}
		char* timer_name = malloc(strlen(argv[2]) + 10);
		sprintf(timer_name, "%s.timer", argv[2]);
		int res = remove(timer_name);
		free(timer_name);
		if (res != 0) {
			err(FILE_REMOVE_FAIL);
			return 0;
		}
		return 0;
	}
	if (arg(1, "rename")) {
		if (argc == 2) {
			err(TYPE_NOT_SPECIFIED);
			printf("tracey rename <TYPE> <OLD> <NEW>\n");
			return 0;
		}
		if (arg(2, "timer")) {
			if (argc == 3) {
				err(TIMER_NOT_SPECIFIED);
				printf("tracey rename timer <OLD> <NEW>\n");
				return 0;
			}
			if (argc == 4) {
				err(NEW_NOT_SPECIFIED);
				printf("tracey rename timer %s <NEW>\n", argv[3]);
				return 0;
			}
			char* old = malloc(strlen(argv[3]) + 10);
			char* new = malloc(strlen(argv[4]) + 10);
			sprintf(old, "%s.timer", argv[3]);
			sprintf(new, "%s.timer", argv[4]);
			rename(old, new);
			free(old);
			free(new);
			return 0;	
		}
		if (arg(2, "item")) {
			if (argc == 3) {
				err(ITEM_NOT_SPECIFIED);
				printf("tracey rename item <OLD> <NEW>\n");
				return 0;
			}
			if (argc == 4) {
				err(NEW_NOT_SPECIFIED);
				printf("tracey rename item %s <NEW>\n", argv[3]);
				return 0;
			}
			char* old = malloc(strlen(argv[3]) + 10);
			char* new = malloc(strlen(argv[4]) + 10);
			sprintf(old, "%s.timer", argv[3]);
			sprintf(new, "%s.timer", argv[4]);
			rename(old, new);
			free(old);
			free(new);
			return 0;
		}
		err(TYPE_INVALID);
		return 0;
	}
	time_t now = time(NULL);
	struct tm *local = localtime(&now);
	char* name = malloc(64);
	sprintf(name, "%02d-%02d-%04d.trace", local->tm_mday, local->tm_mon + 1, local->tm_year + 1900);

	FILE* fileptr = fopen(name, "r+");
	if (fileptr == NULL) {
		err(FILE_OPEN_FAIL);
		free(name);
		return 0;
	}
	trace file = read_file(fileptr);
	fclose(fileptr);
	free_trace(file);
	free(name);
	return 0;
}
