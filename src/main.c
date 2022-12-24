#include <stdio.h>
#include <string.h> // strrchr
#include <stdlib.h> // system

#include "parser.h"
#include "tree.h"
#include "gen.h"
#include "path.h"

static char *string_dup(const char *string) {
	const size_t len = strlen(string) + 1;
	char *data = malloc(len);
	if (!data) return 0;
	return memcpy(data, string, len);
}

typedef struct Command Command;

struct Command {
	const char *name;
	const char *description;
};

static const Command COMMANDS[] = {
	{ "build",   "compile directory of .odin files, as an executable\n\t\t\tone must contain the program's entry point, all must be the same package." },
	{ "run",     "same as 'build', but also runs the newly compiled executable." },
	{ "dump",    "dump the generated syntax tree." },
	{ "version", "print version information" },
};

static char *project_name(const char *path) {
	const char *stem = strrchr(path, '/');
	if (!stem) stem = strrchr(path, '\\');
	char *name = string_dup(stem ? stem : path);
	char *odin = strstr(name, ".odin");
	if (odin) *odin = '\0';
	return name;
}

static int usage(const char *app) {
	printf("Usage:\n");
	printf("\t%s command [arguments]\n", app);
	printf("Commands:\n");
	for (Uint32 i = 0; i < sizeof(COMMANDS)/sizeof(*COMMANDS); i++) {
		const Command *command = &COMMANDS[i];
		printf("\t%s\t\t%s\n", command->name, command->description);
	}
	printf("\n");
	printf("For further details on a command, invoke command help:\n");
	printf("\te.g. `%s build -help` or `%s help build\n", app, app);
	return 1;
}

static Bool build(const char *path, Bool is_file) {
	if (!is_file) {
		fprintf(stderr, "Compiling directories is not yet supported. Use -file.\n");
		return false;
	}

	Tree *tree = parse(path);
	if (!tree) {
		// PARSE ERROR.
		return false;
	}

	Generator gen;
	gen_init(&gen, tree);

	StrBuf strbuf;
	strbuf_init(&strbuf);
	if (!gen_run(&gen, &strbuf)) {
		// Could not generate C0 code.
		fprintf(stderr, "Failed to generate C0\n");
		strbuf_free(&strbuf);
		tree_free(tree);
		return false;
	}

	tree_free(tree);

	char *project = project_name(path);
	// Write this source file out to ".build/%s.c"
	StrBuf file;
	strbuf_init(&file);
	strbuf_put_formatted(&file, ".build/%s.c", project);
	strbuf_put_rune(&file, '\0');

	// Ensure a build directory exists to shove the generated C0.
	path_mkdir(".build");

	const String filename = strbuf_result(&file);
	FILE *fp = fopen(CAST(const char *, filename.data), "wb");
	if (!fp) {
		fprintf(stderr, "Failed to write C0\n");
		free(project);
		strbuf_free(&file);
		strbuf_free(&strbuf);
		return false;
	}
	strbuf_free(&file);

	const String source = strbuf_result(&strbuf);
	fwrite(source.data, source.size, 1, fp);
	fclose(fp);

	strbuf_clear(&strbuf);

	// Generate one of the following commands to build the C0.
	//
	//	"gcc .build/%s.c -O1 -o %s" or
	//	"cl.exe .build/%s.c /O1 /OUT:%s"
#if defined(OS_WINDOWS)
	strbuf_put_formatted(&strbuf, "cl.exe .build/%s.c /O1 /OUT:%s", project, project);
#elif defined(OS_LINUX)
	strbuf_put_formatted(&strbuf, "gcc .build/%s.c -O1 -o %s", project, project);
#endif
	strbuf_put_rune(&strbuf, '\0');
	free(project);

	// Compile it.
	const String compile = strbuf_result(&strbuf);
	const Bool status = system(CAST(const char *, compile.data)) == 0;
	strbuf_free(&strbuf);

	return status;
}

static Bool run(const char *path) {
	char *project = project_name(path);
	const Bool result = system(project) == 0;
	free(project);
	return result;
}

static Bool dump(const char *file) {
	Tree *tree = parse(file);
	if (tree) {
		tree_dump(tree);
		tree_free(tree);
		return true;
	}
	return false;
}

int main(int argc, char **argv) {
	const char *app = argv[0];

	argc--;
	argv++;

	if (argc <= 1) {
		return usage(app);
	}

	const Bool file = argc >= 3 && !strcmp(argv[2], "-file");

	if (!strcmp(argv[0], "build")) {
		return build(argv[1], file) ? 0 : 1;
	} else if (!strcmp(argv[0], "run")) {
		return (build(argv[1], file) && run(argv[1])) ? 0 : 1;
	} else if (!strcmp(argv[0], "dump")) {
		return dump(argv[1]) ? 0 : 1;
	}

	return usage(app);
}