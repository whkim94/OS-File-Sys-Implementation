#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <uthread.h>
#include <fs.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define test_fs_error(fmt, ...) \
	fprintf(stderr, "%s: "fmt"\n", __func__, ##__VA_ARGS__)

#define die(...)			\
do {					\
	test_fs_error(__VA_ARGS__);	\
	exit(1);			\
} while (0)


struct thread_arg {
	int argc;
	char **argv;
};

void thread_fs_rm(void *arg)
{
	struct thread_arg *t_arg = arg;
	size_t i;
	char *diskname;

	if (t_arg->argc < 1)
		die("need <diskname> argument");

	diskname = t_arg->argv[0];

	if (fs_mount(diskname))
		die("cannot mount diskname");

	for (i = 1; i < t_arg->argc; i++) {
		char *filename = t_arg->argv[i];

		printf("Removing file '%s' from file system\n", filename);

		if (fs_delete(filename)) {
			test_fs_error("failed to delete file");
			break;
		}
	}

	if (fs_umount())
		die("cannot unmount diskname");
}

void thread_fs_add(void *arg)
{
	struct thread_arg *t_arg = arg;
	size_t i;
	char *diskname;

	if (t_arg->argc < 1)
		die("need <diskname> argument");

	diskname = t_arg->argv[0];

	if (fs_mount(diskname))
		die("cannot mount diskname");

	for (i = 1; i < t_arg->argc; i++) {
		int fd;
		char *filename = t_arg->argv[i];

		fd = open(filename, O_RDONLY);
		if (fd < 0) {
			perror("open");
			exit(1);
		}

		printf("Adding file '%s' to file system\n", filename);

		if (fs_create(filename)) {
			test_fs_error("failed to create file");
			break;
		}
	}

	if (fs_umount())
		die("cannot unmount diskname");
}

void thread_fs_ls(void *arg)
{
	struct thread_arg *t_arg = arg;
	char *diskname;

	if (t_arg->argc < 1)
		die("need <diskname> argument");

	diskname = t_arg->argv[0];

	if (fs_mount(diskname))
		die("cannot mount diskname");

	fs_ls();

	if (fs_umount())
		die("cannot unmount diskname");
}

void thread_fs_info(void *arg)
{
	struct thread_arg *t_arg = arg;
	char *diskname;

	if (t_arg->argc < 1)
		die("need <diskname> argument");

	diskname = t_arg->argv[0];

	if (fs_mount(diskname))
		die("cannot mount diskname");

	fs_info();

	if (fs_umount())
		die("cannot unmount diskname");
}

static struct {
	const char *name;
	uthread_func_t func;
} commands[] = {
	{ "info",	thread_fs_info },
	{ "ls",		thread_fs_ls },
	{ "add",	thread_fs_add },
	{ "rm",		thread_fs_rm },
};

void usage(void)
{
	int i;
	fprintf(stderr, "Usage: test-fs <command> [<arg>]\n");
	fprintf(stderr, "Possible commands are:\n");
	for (i = 0; i < ARRAY_SIZE(commands); i++)
		fprintf(stderr, "\t%s\n", commands[i].name);
	exit(1);
}

int main(int argc, char **argv)
{
	int i;
	char *cmd;
	struct thread_arg arg;

	/* Skip argv[0] */
	argc--;
	argv++;

	if (!argc)
		usage();

	cmd = argv[0];
	arg.argc = --argc;
	arg.argv = &argv[1];

	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (!strcmp(cmd, commands[i].name)) {
			uthread_start(commands[i].func, &arg);
			break;
		}
	}
	if (i == ARRAY_SIZE(commands)) {
		test_fs_error("invalid command '%s'", cmd);
		usage();
	}

	return 0;
}
