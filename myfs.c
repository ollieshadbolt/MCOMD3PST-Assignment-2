#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

int max_disksize = 512;

int help(char command[]);
int store(char input_filepath[]);
int retrieve(char stored_filename[], char output_filepath[]);
int del(char stored_filename[]);
int check_argc(int valid_argc, int argc);
int logging(char log[]);

/* Wrapper */
int main(int argc, char* argv[])
{
	int error_code_one;
	int error_code_two;
	char log[512];

	/* Handling zero additional arguments */
	if (argc == 1)
	{
		error_code_one = logging("Program called without any arguments");

		/* logging() error handling */
		if (error_code_one != 0)
		{
			return error_code_one;
		}
	}
	else
	{
		/* Switch string */
		if (strcmp(argv[1], "store") == 0)
		{
			/* Store */
			if (check_argc(3, argc) == 0)
			{
				error_code_one = store(argv[2]);

				if (error_code_one == 0)
				{
					return 0;
				}

				snprintf(log, sizeof(log), "'%s' not able to be stored", argv[2]);
			}
		}
		else if (strcmp(argv[1], "retrieve") == 0)
		{
			/* Retrieve */
			if (check_argc(4, argc) == 0)
			{
				error_code_one = retrieve(argv[2], argv[3]);

				if (error_code_one == 0)
				{
					snprintf(log, sizeof(log), "'%s' retrieved successfully to '%s'", argv[2], argv[3]);
					return logging(log);
				}

				snprintf(log, sizeof(log), "'%s' not retrieved successfully to '%s'", argv[2], argv[3]);
			}
		}
		else if (strcmp(argv[1], "delete") == 0)
		{
			/* Delete */
			if (check_argc(3, argc) == 0)
			{
				error_code_one = del(argv[2]);

				if (error_code_one == 0)
				{
					snprintf(log, sizeof(log), "'%s' deleted successfully", argv[2]);
					return logging(log);
				}

				snprintf(log, sizeof(log), "'%s' not deleted successfully", argv[2]);
			}
		}

		/* Check if any function failed and return appropriate error code */
		if (error_code_one != 0)
		{
			error_code_two = logging(log);

			if (error_code_two == 0)
			{
				return error_code_one;
			}

			return error_code_two;
		}
	}

	/* Invalid arguments */
	return help(argv[0]);
}

/* Basic help */
int help(char command[])
{
	int error_code;

	error_code = printf(
	"Store or update a text file\n"
	"%1$s store filename.txt\n"
	"\n"
	"Retrieve stored file\n"
	"%1$s retrieve filename.txt output.txt\n"
	"\n"
	"Delete a file\n"
	"%1$s delete filename.txt\n"
	, command);

	/* printf() error handling */
	if (error_code < 0)
	{
		return error_code;
	}
	else
	{
		return 1;
	}
}

/* Store or update a text file */
int store(char input_filepath[])
{
	int error_code;
	int disksize;
	int stored_filesize;
	int i;
	FILE *input_file;
	FILE *stored_file;
	char diskpath[512];
	char stored_filepath[512];
	char log[512];
	char input_char;
	char *last;
	char *input_filename;
	DIR* dir;
	struct dirent *stored_filename;

	input_file = fopen(input_filepath, "r");

	/* fopen() error handling */
	if (input_file == NULL)
	{
		return 1;
	}

	/* Get filename from input_path */
	last = strrchr(input_filepath, '/');

	if (last != NULL)
	{
		input_filename = last + 1;
	}
	else
	{
		input_filename = input_filepath;
	}

	error_code = del(input_filename);

	/* del() error handling */
	if (error_code != 0)
	{
		return error_code;
	}

	/* Loop for each disk */
	for (i = 0; i >= 0; i++)
	{
		snprintf(diskpath, sizeof(diskpath), "DISK%i", i);
		mkdir(diskpath, 0700);
		dir = opendir(diskpath);
		
		/* opendir() error handling */
		if (dir == NULL)
		{
			return 1;
		}

		disksize = max_disksize;

		/* Get disk size */
		while ((stored_filename = readdir(dir)) != NULL)
		{
			snprintf(stored_filepath, sizeof(stored_filepath), "%s/%s", diskpath, stored_filename->d_name);
			stored_file = fopen(stored_filepath, "r");

			/* fopen() error handling */
			if (stored_file == NULL)
			{
				return 1;
			}

			fseek(stored_file, 0L, SEEK_END);
			stored_filesize = ftell(stored_file);

			if (stored_filesize > 0)
			{
				disksize -= stored_filesize;
			}

			if (fclose(stored_file) == EOF)
			{
				return 1;
			}
		}

		/* No space on disk */
		if (disksize <= 0)
		{
			continue;
		}

		snprintf(stored_filepath, sizeof(stored_filepath), "%s/%s", diskpath, input_filename);
		stored_file = fopen(stored_filepath, "w");

		/* fopen() error handling */
		if (stored_file == NULL)
		{
			return 1;
		}

		/* Write as much possible to stored_file */
		while (disksize > 0)
		{
			input_char = fgetc(input_file);
			disksize--;

			if (input_char == EOF)
			{
				break;
			}

			if (fprintf(stored_file, "%c", input_char) == EOF)
			{
				return 1;
			}
		}

		snprintf(log, sizeof(log), "'%s' stored successfully as '%s' in '%s'", input_filepath, input_filename, diskpath);
		error_code = logging(log);

		/* logging() error handling */
		if (error_code != 0)
		{
			return error_code;
		}

		if (fclose(stored_file) == EOF)
		{
			fclose(input_file);
			return 1;
		}

		if (input_char == EOF)
		{
			if (fclose(input_file) == EOF)
			{
				return 1;
			}

			return 0;
		}
	}

	return 1;
}

/* Retrieve stored files */
int retrieve(char stored_filename[], char output_filepath[])
{
	FILE *output_file;
	FILE *stored_file;
	char diskpath[128];
	char stored_filepath[128];
	char stored_char;
	DIR *dir;
	int i;
	int error;

	output_file = fopen(output_filepath, "w");

	/* fopen() error checking */
	if (output_file == NULL)
	{
		return 1;
	}

	/* Loop for each disk */
	for (i = 0; i >= 0; i++)
	{
		snprintf(diskpath, sizeof(diskpath), "DISK%i", i);
		dir = opendir(diskpath);

		if (dir)
		{
			/* Directory exists */
			snprintf(stored_filepath, sizeof(stored_filepath), "%s/%s", diskpath, stored_filename);
			stored_file = fopen(stored_filepath, "r");

			if (stored_file == NULL)
			{
				continue;
			}

			stored_char = fgetc(stored_file);

			/* Get the stored file */
			while (stored_char != EOF)
			{
				fprintf(output_file, "%c", stored_char);
				stored_char = fgetc(stored_file);
			}

			/* fclose() error checking */
			if (fclose(stored_file) == EOF)
			{
				return 1;
			}
		}
		else if (ENOENT == errno)
		{
			/* Directory doesn't exist */
			if (fclose(output_file) == EOF)
			{
				return 1;
			}

			return 0;
		}
		else
		{
			/* Error with opening directory */
			fclose(output_file);
			return 1;
		}
	}

	return 1;
}

/* Delete a file */
int del(char stored_filename[])
{
	int i;
	char diskpath[128];
	char stored_filepath[128];
	DIR *dir;
	int error_code;

	/* For each disk */
	for (i = 0; i >= 0; i++)
	{
		snprintf(diskpath, sizeof(diskpath), "DISK%i", i);
		dir = opendir(diskpath);

		if (dir)
		{
			/* Directory exists */
			error_code = snprintf(stored_filepath, sizeof(stored_filepath), "%s/%s", diskpath, stored_filename);

			/* snprintf() error handling */
			if (error_code < 0)
			{
				closedir(dir);
				return error_code;
			}

			remove(stored_filepath);
			closedir(dir);
		}
		else if (ENOENT == errno)
		{
			/* Directory doesn't exist */
			return 0;
		}
		else
		{
			/* Error with opening directory */
			return 1;
		}
	}

	/* Loop error */
	return 1;
}

/* Detailed logging */
int logging(char log[])
{
	int error_code;
	time_t rawtime;
	struct tm *now;
	char dir[8];
	char filename[16];
	char filepath[32];
	char timestamp[32];
	FILE *file;

	error_code = printf("%s\n", log);

	/* print() error handling */
	if (error_code < 0)
	{
		return error_code;
	}

	strcpy(dir, "logs");
	rawtime = time(NULL);
	now = localtime(&rawtime);
	strftime(filename, sizeof(filename), "%Y-%m-%d.log", now);
	error_code = snprintf(filepath, sizeof(filepath), "%s/%s", dir, filename);

	/* snprintf() error handling */
	if (error_code < 0)
	{
		return error_code;
	}

	strftime(timestamp, sizeof(timestamp), "%x %X", now);
	mkdir(dir, 0700);
	file = fopen(filepath, "a");

	/* fopen() error handling */
	if (file == NULL)
	{
		return 1;
	}

	error_code = fprintf(file, "[%s] %s\n", timestamp, log);

	/* fprintf() error handling */
	if (error_code < 0)
	{
		return error_code;
	}

	/* fclose() error handling */
	if (fclose(file) == EOF)
	{
		return 1;
	}

	return 0;
}

/* Check if argc is valid */
int check_argc(int valid_argc, int argc)
{
	int error_code;

	if (valid_argc == argc)
	{
		return 0;
	}

	error_code = printf("Expected %d arguments, got %d instead\n", valid_argc, argc);

	/* printf() error handling */
	if (error_code < 0)
	{
		return error_code;
	}

	return 1;
}
