#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

#include <string.h>

static char buffer[2048];

char* readline(char* prompt) {
	char* copy;
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	copy = malloc(strlen(buffer) + 1);
	strcpy(copy, buffer);
	copy[strlen(copy) - 1] = '\0';
	return copy;
}

void add_history(char* str) {}

#else

#include <editline/readline.h>

#endif

int main(int argc, char *argv[]) {
	puts("Lispora version 0.0.0.3");
	puts("Press Ctrl+C to exit.\n");

	while(1) {
		char* input = readline("lispora> ");
		add_history(input);
		printf("%s\n", input);

		free(input);
	}

	return 0;
}
