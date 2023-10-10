#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <libgen.h>

#define PROGRAM_NAME "tts"
#define BUF_LEN 16

/* to convert from say /tmp/foo.txt to FOO_TXT_H_ */
static char *get_header_macro(const char *file_name)
{
        size_t namelen;
        char *header_name, *name, *upper;
        char c;
        size_t i;

        name = basename((char *) file_name);
        assert(name != NULL);
        namelen = strlen(name);

        /* convert to uppercase */
        /* NOTE: ALWAYS ALLOCATE AN EXTRA BYTE FOR THE NULL BYTE!!1!1 */
        upper = malloc(namelen * sizeof(char) + 1);
        memset(upper, 0, namelen + 1);

        /* for loop is safer */
        for(i = 0; i < namelen; i++) {
                c = name[i];
                if(c >= 'a' && c <= 'z')
                        upper[i] = c - ('a' - 'A');
                else {
                        switch (c) {
                        case '.':
                                upper[i] = '_';
                                break;
                        case '/':
                                assert(0 && "This should never happen.");
                                break;
                        default:
                                upper[i] = c;
                                break;
                        }
                }
        }

        header_name = malloc(BUF_LEN * sizeof(char));
        memset(header_name, 0, BUF_LEN);
        snprintf(header_name, BUF_LEN, "%s_H_", upper);

        free(upper);

        return header_name;
}

/* this whole function is a massive HACK */
static void convert_to_header(const char *input, const char *name)
{
        char *header_macro, *str_name;
        char *prototype =
                "#ifndef %s\n"
                "#define %s\n"
                "\n"
                "const char *%s =\n";

        /* change . to _ in  file name */
        {
                char *period_pos, *base_name;
                size_t namelen;

                base_name = basename((char *)name);
                namelen = strlen(base_name);

                str_name = malloc(namelen * sizeof(char) + 1);
                strncpy(str_name, base_name, namelen + 1);
                period_pos = strchr(str_name, '.');
                /* for now, let's assume there's only a single period in a path */
                if (period_pos)
                        *period_pos = '_';
        }

        header_macro = get_header_macro(name);
        assert(header_macro != NULL);

        printf(prototype, header_macro, header_macro, str_name);

        {
                char *line_copy, *newline_pos, *start;
                size_t line_len, i = 0;
                size_t num_lines = 0;

                /* some reconnaisance work first */
                start = (char *) input;
                newline_pos = strchr(start, '\n');
                do {
                        line_len = newline_pos - start + 1;
                        start = newline_pos + 1;
                        newline_pos = strchr(start, '\n');
                        num_lines++;
                } while(newline_pos);

                /* length of the file + two quotes + newline per line + null byte */
                #if 0
                output_size = strlen(input) + (num_lines * 3) + 1;
                #endif

                /* start all over again */
                start = (char *) input;
                newline_pos = strchr(start, '\n');
                do{
                        /* don't forget the newline! */
                        line_len = newline_pos - start + 1;
                        /* we need to make a copy of the line */
                        line_copy = malloc(line_len);
                        strncpy(line_copy, start, line_len);
                        /* remove the trailing bytes at the end of the line */
                        *(line_copy + line_len - 1) = '\0';

                        printf("\t\"%s\\n\"", line_copy);
                        if(i == num_lines - 1)
                                putchar(';');
                        putchar('\n');

                        start = newline_pos + 1;
                        newline_pos = strchr(start, '\n');
                        i++;

                        free(line_copy);
                } while(newline_pos);
        }

        printf("\n#endif /* %s */\n", header_macro);


        free(str_name);
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
        if(file_length == 0) {
                fprintf(stderr, "error: file is 0 bytes long\n");
                return EXIT_FAILURE;
        }

        contents = malloc(file_length * sizeof(char));
        for(i = 0; i < file_length; i++)
                contents[i] = fgetc(fp);
        assert(fgetc(fp) == EOF && i == file_length);
        fclose(fp);

        convert_to_header(contents, file_name);

        free(contents);

        return EXIT_FAILURE;
}
