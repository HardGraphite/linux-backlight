#include <assert.h>
#include <limits.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#define EXIT_FAIL_ARGS  1 // Argument error.
#define EXIT_FAIL_PERM  2 // Permission denied.
#define EXIT_FAIL_FILE  3 // Cannot find file.

#define BACKLIGHT_BASE_DIR   "/sys/class/backlight"
#define MAX_BRIGHTNESS_FILE  "max_brightness"
#define BRIGHTNESS_FILE      "brightness"

struct args {
	enum {
		OP_SET,
		OP_INC,
		OP_DEC,
		OP_GET,
	}             op;
	unsigned long val;
};

static void print_help(const char *prog_name, FILE *out) {
	fputs("Usage:\n", out);
	fprintf(out, "\t%s %-20s# %s\n", prog_name, ""          , "get brightness");
	fprintf(out, "\t%s %-20s# %s\n", prog_name, "=<PERCENT>", "set brightness");
	fprintf(out, "\t%s %-20s# %s\n", prog_name, "+<PERCENT>", "increase brightness");
	fprintf(out, "\t%s %-20s# %s\n", prog_name, "-<PERCENT>", "decrease brightness");
	fputc('\n', out);
	fputs("Control screen brightness.\n", out);
}

static struct args parse_args(int argc, char *argv[]) {
	struct args args;
	if (argc == 1) {
		args.op  = OP_GET;
		args.val = 0;
	} else if (argc == 2) {
		const char *cmd_str = argv[1];
		const char *const cmd_str_end = cmd_str + strlen(cmd_str);
		const char op_ch = cmd_str[0];
		if (op_ch == '=')
			args.op = OP_SET;
		else if (op_ch == '+')
			args.op = OP_INC;
		else if (op_ch == '-')
			args.op = OP_DEC;
		else
			goto bad_args;
		char *end_p;
		args.val = strtoul(cmd_str + 1, &end_p, 10);
		if ((args.val == 0 && !(cmd_str[1] == '0' && cmd_str[2] == 0))
				|| end_p != cmd_str_end)
			goto bad_args;
	} else {
	bad_args:
		fprintf(stderr, "%s: %s", argv[0], "unexpected arguments\n");
		print_help(argv[0], stdout);
		exit(EXIT_FAIL_ARGS);
	}
	return args;
}

static char *path_concat(char *path1, const char *path2) {
	size_t path1_size = strlen(path1);
	if (path1_size && path1[path1_size - 1] != '/')
		path1[path1_size++] = '/';
	strcpy(path1 + path1_size, path2);
	return path1;
}

static char *path_concat2(char buffer[], const char *path1, const char *path2) {
	strcpy(buffer, path1);
	return path_concat(buffer, path2);
}

static void find_backlight_dir(char *dir_path) {
	strcpy(dir_path, BACKLIGHT_BASE_DIR);

	DIR *dir = opendir(dir_path);
	if (!dir) {
		fprintf(stderr, "directory does not exist: %s\n", dir_path);
		exit(EXIT_FAIL_FILE);
	}
	while (1) {
		const struct dirent *const dirent = readdir(dir);
		if (!dirent) {
			closedir(dir);
			fprintf(stderr, "cannot find sub-dir in %s\n", dir_path);
			exit(EXIT_FAIL_FILE);
		}
		if ((dirent->d_type == DT_DIR || dirent->d_type == DT_LNK )
				&& dirent->d_name[0] != '.') {
			path_concat(dir_path, dirent->d_name);
			break;
		}
	}
	closedir(dir);
}

static unsigned long read_file_ulong(const char *file) {
	char buf[64];
	int fd = open(file, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "cannot read from file: %s\n", file);
		exit(EXIT_FAIL_FILE);
	}
	const ssize_t nrd = read(fd, buf, sizeof buf - 1);
	close(fd);
	assert(nrd > 0);
	buf[nrd] = 0;
	return (unsigned long)atol(buf);
}

static size_t write_file_ulong(unsigned long val, const char *file) {
	char buf[64];
	const int len = snprintf(buf, sizeof buf, "%lu", val);
	assert(len > 0);
	int fd = open(file, O_WRONLY);
	if (fd == -1) {
		fprintf(stderr, "cannot write to file: %s\n", file);
		exit(EXIT_FAIL_FILE);
	}
	const ssize_t nwr = write(fd, buf, (size_t)len);
	close(fd);
	assert(nwr > 0);
	return (size_t)nwr;
}

static char backlight_dir[PATH_MAX];
static char path_buffer[PATH_MAX];

int main(int argc, char *argv[]) {
	const struct args args = parse_args(argc, argv);
	find_backlight_dir(backlight_dir);

	const unsigned long br_max = read_file_ulong(
		path_concat2(path_buffer, backlight_dir, MAX_BRIGHTNESS_FILE));
	const unsigned long br_old = read_file_ulong(
		path_concat2(path_buffer, backlight_dir, BRIGHTNESS_FILE));
	const double br_old_ratio = (double)br_old / br_max;
	double br_new_ratio;

	switch (args.op) {
	case OP_GET:
		br_new_ratio = br_old_ratio;
		break;
	case OP_SET:
		br_new_ratio = args.val / 100.0;
		break;
	case OP_INC:
		br_new_ratio = br_old_ratio + args.val / 100.0;
		break;
	case OP_DEC:
		br_new_ratio = br_old_ratio - args.val / 100.0;
		break;
	default:
		abort();
	}

	if (br_new_ratio > 1.0)
		br_new_ratio = 1.0;
	else if (br_new_ratio < 0.0)
		br_new_ratio = 0.0;
	const unsigned long br_new = (unsigned long)(br_new_ratio * br_max);
	assert(br_new <= br_max);

	if (br_new != br_old) {
		const int orig_uid = getuid();
		if (setuid(0) != 0) {
			fprintf(stderr, "setuid() fails; "
				"check owner and setuid bit of program %s\n", argv[0]);
			exit(EXIT_FAIL_PERM);
		}
		write_file_ulong(
			br_new, path_concat2(path_buffer, backlight_dir, BRIGHTNESS_FILE));
		setuid(orig_uid);
	}
	printf("%i%%\n", (int)(br_new_ratio * 100.0));
}

