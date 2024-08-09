#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

int
main(int argc, char *argv[])
{
    int number;
    int start = 0;
    int i = 0;
    char *filename;
    char *output;
    int max_level;
    int show_error = 0;

    if (argc < 2) {
        printf("Usage: %s file1.yaml ...\n", argv[0]);
        return 0;
    }
    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--max-level", 11) == 0) {
            i++;
            max_level = strtol(argv[i], &output, 10);
            yaml_set_max_nest_level(max_level);
            start = i+1;
        }
        else if (strncmp(argv[i], "--show-error", 12) == 0) {
            show_error = 1;
            start = i+1;
        }
    }

    for (number = start; number < argc; number ++)
    {
        FILE *file;
        yaml_parser_t parser;
        yaml_event_t event;
        int done = 0;
        int count = 0;
        int error = 0;

        filename = argv[number];
        printf("[%d] Parsing '%s': ", number, filename);
        fflush(stdout);

        file = fopen(filename, "rb");
        assert(file);

        assert(yaml_parser_initialize(&parser));

        yaml_parser_set_input_file(&parser, file);

        while (!done)
        {
            if (!yaml_parser_parse(&parser, &event)) {
                error = 1;
                if (show_error) {
                    fprintf(stderr, "Parse error: %s\nLine: %lu Column: %lu\n",
                        parser.problem,
                        (unsigned long)parser.problem_mark.line + 1,
                        (unsigned long)parser.problem_mark.column + 1);
                }
                break;
            }

            done = (event.type == YAML_STREAM_END_EVENT);

            yaml_event_delete(&event);

            count ++;
        }

        yaml_parser_delete(&parser);

        assert(!fclose(file));

        printf("%s (%d events)\n", (error ? "FAILURE" : "SUCCESS"), count);
    }

    return 0;
}

