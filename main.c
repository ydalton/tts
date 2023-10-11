#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <libgen.h>

#define PROGRAM_NAME "tts"
#define BUF_LEN 16

/* converts a string to uppercase */
static char *strupper(const char* src)
{
        size_t outlen;
        char *output;
        size_t i;
        char c;

        outlen = strlen(src) + 1;

        output = malloc(outlen * sizeof(char));
        strncpy(output, src, outlen);
        for(i = 0; i < outlen - 1; i++) {
                c = output[i];
                if(c >= 'a' && c <= 'z')
                        output[i] = c - ('a' - 'A');
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

        header_name = malloc(BUF_LEN * sizeof(char));
        strncpy(header_name, upper, BUF_LEN);
        strncat(header_name, "_H_", BUF_LEN);

        free(upper);

        return header_name;
}

static int print_formatted_file_contents(const char *input,
                                          const char *header_macro)
{
        char *line_copy, *newline_pos, *start;
        size_t line_len, i = 0;
        size_t num_lines = 0;

        /* some reconnaisance work first */
        start = (char *)input;
        newline_pos = strchr(start, '\n');
        /* either an empty file or a very weird file */
        if (newline_pos == NULL) {
                printf("\t\"%s\";\n"
                       "\n#endif /* %s */\n",
                       start, header_macro);
                return -1;
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

                printf("\t\"%s\\n\"", line_copy);
                if (i == num_lines - 1)
                        putchar(';');
                putchar('\n');

                start = newline_pos + 1;
                newline_pos = strchr(start, '\n');
                i++;

                free(line_copy);
        } while (newline_pos);

        return 0;
}

/* this whole function is a massive HACK */
static void convert_to_header(const char *input, const char *name)
{
        char *header_macro, *str_name, *base_name;
        char *prototype =
                "#ifndef %s\n"
                "#define %s\n"
                "\n"
                "const char *%s =\n";
        size_t namelen;
        int ret;

        /* change . to _ in  file name */
        base_name = basename((char *)name);
        namelen = strlen(base_name);

        str_name = malloc(namelen * sizeof(char) + 1);
        strncpy(str_name, base_name, namelen + 1);

        period_to_underscore(str_name);

        header_macro = get_header_macro(name);
        assert(header_macro != NULL);

        printf(prototype, header_macro, header_macro, str_name);
        free(str_name);

        /* returns -1 if there is no newline at all in the file */
        ret = print_formatted_file_contents(input, header_macro);
        if(ret)
                goto out;

        printf("\n#endif /* %s */\n", header_macro);

out:
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

static void usage(int code)
{
        char *program_name = PROGRAM_NAME;
        printf("usage: %s FILE\n"
               "Converts a plain text file into a C header containing\na string "
               "with the contents of that file.\n", program_name);
        exit(code);
}

int main(int argc, char **argv)
{
        FILE *fp;
        char *file_name, *contents;
        size_t i, file_length;

        if(argc != 2)
                usage(-1);

        file_name = argv[1];

        fp = fopen(file_name, "r");
        if(!fp) {
                perror("Cannot open file");
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

        convert_to_header(contents, file_name);

        free(contents);

        return EXIT_FAILURE;
}
