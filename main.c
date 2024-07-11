#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <libgen.h>

#define BUF_LEN 16

#define FATAL(err) fprintf(stderr, "%s: %s: %s\n", invoked_name, file_name, strerror(err))

int read_binary = 0;

char *invoked_name;

/* converts a string to uppercase */
static char *strupper(const char* src)
{
        char *output;
        size_t i;
        int c;

        output = strdup(src);
        for(i = 0; i < strlen(src); i++) {
                c = output[i];
                if(islower(c))
                        output[i] = toupper(c);
        }

        return output;
}

/*
 * convenience function
 *
 * NOTE: this function is ridiculously unsafe. do not feed it a statically
 * declared string, or bad things will happen!
 */
static void period_to_underscore(const char *s)
{
        char *period_pos;

        period_pos = strchr(s, '.');
        while(period_pos) {
                *period_pos = '_';
                period_pos = strchr(period_pos, '.');
        }
}

/* to convert from, say, /tmp/foo.txt to FOO_TXT_H_ */
static char *get_header_macro(const char *file_name)
{
        char *header_name, *name, *upper;

        name = basename((char *) file_name);
        assert(name != NULL);

        /* convert to uppercase */
        /* NOTE: ALWAYS ALLOCATE AN EXTRA BYTE FOR THE NULL BYTE!!1!1 */
        upper = strupper(name);

        period_to_underscore(upper);

        header_name = malloc(BUF_LEN * sizeof(char) + 1);
        strncpy(header_name, upper, BUF_LEN);
        strncat(header_name, "_H_", BUF_LEN);

        free(upper);

        return header_name;
}

static void print_formatted_file_contents(FILE *out, const char *input, size_t len)
{
        char *line_copy, *newline_pos, *start;
        size_t line_len, i = 0;
        size_t num_lines = 0;
        /* for the sake of ignoring the compiler warning, i will count the
         * number of bytes printed, then compare it with the length of the file
         */
        size_t bytes_printed = 0;

        /* some reconnaisance work first */
        start = (char *)input;
        newline_pos = strchr(start, '\n');
        /* either an empty file or a very weird file */
        if (newline_pos == NULL) {
                bytes_printed += fprintf(out, "\t\"%s\";\n", start) - 1;
                return;
        }

        do {
                line_len = newline_pos - start + 1;
                start = newline_pos + 1;
                newline_pos = strchr(start, '\n');
                num_lines++;
        } while (newline_pos);

        /* start all over again */
        start = (char *)input;
        newline_pos = strchr(start, '\n');
        do {
                /* don't forget the newline! */
                line_len = newline_pos - start + 1;
                /* we need to make a copy of the line */
                line_copy = malloc(line_len);
                strncpy(line_copy, start, line_len);
                /* remove the trailing bytes at the end of the line */
                *(line_copy + line_len - 1) = '\0';

                bytes_printed += fprintf(out, "\t\"%s\\n\"", line_copy) - 1;
                if (i == num_lines - 1)
                        fputc(';', out);
                fputc('\n', out);

                start = newline_pos + 1;
                newline_pos = strchr(start, '\n');
                i++;

                free(line_copy);
        } while (newline_pos);
        assert(bytes_printed > len);
}

static void print_binary_file_contents(FILE *out, const char *input, size_t len)
{
        size_t i;
        fprintf(out, "\t{");
        for(i = 0; i < len; i++) {
                if(i % 10 == 0 && i != 0)
                        fprintf(out, "\n\t");
                fprintf(out, "0x%02x", input[i]);
                /* because puts adds a newline that you cannot disable */
                if(i != len - 1)
                        fputc(',', out);
        }
        fputs("};", out);
}

/* this whole function is a massive HACK */
static void convert_to_header(FILE *out, const char *input, const char *name, size_t filelen)
{
        char *header_macro, *str_name;
        char *prototype =
                "#ifndef %s\n"
                "#define %s\n"
                "\n";
        char *variable_name;
        /* haha funny pointer trick */
        void (*print_func)(FILE *, const char*, size_t);

        variable_name = (read_binary) ? "const unsigned char %s[] =\n"
                : "const char *%s =\n";

        str_name = strdup(basename((char *)name));

        /* change . to _ in  file name */
        period_to_underscore(str_name);

        header_macro = get_header_macro(name);
        assert(header_macro != NULL);

        fprintf(out, prototype, header_macro, header_macro);
        fprintf(out, variable_name, str_name);
        free(str_name);

        print_func = (read_binary) ? print_binary_file_contents
                : print_formatted_file_contents;

        print_func(out, input, filelen);

        fprintf(out, "\n#endif /* %s */\n", header_macro);

        free(header_macro);
}

static size_t get_file_len(FILE *fp)
{
        size_t len;
        int pos;

        /* store current position */
        pos = ftell(fp);
        fseek(fp, 0L, SEEK_END);
        len = ftell(fp);
        /* restore to original position */
        fseek(fp, 0L, pos);

        return len;
}

static int path_is_dir(const char *path)
{
        struct stat path_stat;
        stat(path, &path_stat);
        return S_ISDIR(path_stat.st_mode);
}

static void usage(int code)
{
        printf("usage: %s [-b] FILE\n"
               "Converts a plain text file into a C header containing\na string "
               "with the contents of that file. If the -b flag\nis set, read the "
               "file as a binary file.\n", invoked_name);
        exit(code);
}

int main(int argc, char **argv)
{
        size_t i, file_length;
        FILE *fp;
        FILE *out = stdout;
        char *file_name, *out_filename = NULL, *contents;
        int c;

        invoked_name = argv[0];

        while((c = getopt(argc, argv, "bo:")) != -1) {
                switch(c) {
                case 'b':
                        read_binary = 1;
                        break;
                case 'o':
                        out_filename = strdup(optarg);
                        break;
                case '?':
                default:
                        usage(-1);
                }
        }

        file_name = argv[optind];
        if(!file_name)
                usage(-1);

        if(path_is_dir(file_name)) {
                FATAL(EISDIR);
                return EXIT_FAILURE;
        }

        fp = fopen(file_name, "r");
        if(!fp) {
                FATAL(errno);
                return EXIT_FAILURE;
        }

        file_length = get_file_len(fp);
        if(file_length == 0)
                fprintf(stderr, "warning: file is 0 bytes long\n");

        contents = malloc(file_length * sizeof(char) + 1);
        for(i = 0; i < file_length; i++)
                contents[i] = fgetc(fp);
        assert(fgetc(fp) == EOF && i == file_length);
        fclose(fp);

        if(out_filename) {
                out = fopen(out_filename, "w");
                assert(out);
        }
        convert_to_header(out, contents, file_name, file_length);

        if(out_filename) {
                fclose(out);
                free(out_filename);
        }

        free(contents);

        return EXIT_SUCCESS;
}
