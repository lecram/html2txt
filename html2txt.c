/* gcc -Igumbo -static -o html2txt html2txt.c -Lgumbo -lgumbo */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "gumbo.h"

static char *
read_stdin()
{
    char buffer[BUFSIZ];
    char *output;
    int extra;
    int length = 0;
    int bulk = 1024;

    output = (char *) malloc(bulk);
    strcpy(output, "");
    while ((extra = fread(buffer, 1, BUFSIZ, stdin))) {
        while (length + extra > bulk - 1) {
            bulk += bulk >> 1;
            output = (char *) realloc(output, bulk);
        }
        strcat(output, buffer);
        length += strlen(buffer);
    }
    return output;
}

static void
read_file(FILE* fp, char** output, int* length)
{
    struct stat filestats;
    int fd = fileno(fp);
    fstat(fd, &filestats);
    *length = filestats.st_size;
    *output = malloc(*length + 1);
    int start = 0;
    int bytes_read;
    while ((bytes_read = fread(*output + start, 1, *length - start, fp)))
        start += bytes_read;
}

static void
print_norm(const char *text)
{
    char *str, *token, *seps, *spaces;
    int length, trailing;

    seps = " \t\n\r";
    length = strlen(text);
    str = (char *) malloc(length + 1);
    strcpy(str, text);
    spaces = str;
    while (isspace(*spaces)) {
        if (*spaces == ' ')
            printf(" ");
        spaces++;
    }
    trailing = 0;
    spaces = str + length - 1;
    while (isspace(*spaces)) {
        if (*spaces == ' ')
            trailing++;
        spaces--;
    }
    token = strtok(str, seps);
    printf(token);
    token = strtok(NULL, seps);
    while (token) {
        printf(" %s", token);
        token = strtok(NULL, seps);
    }
    while (trailing--)
        printf(" ");
    free(str);
}

static void
print_tree(GumboNode *node, int plain)
{
    GumboVector *children;
    GumboAttribute *attr;
    int i;

    if (node->type == GUMBO_NODE_TEXT) {
        if (plain)
            printf(node->v.text.text);
        else
            print_norm(node->v.text.text);
    } else if (
        node->type == GUMBO_NODE_ELEMENT &&
        node->v.element.tag != GUMBO_TAG_SCRIPT &&
        node->v.element.tag != GUMBO_TAG_STYLE
    ) {
        if (node->v.element.tag == GUMBO_TAG_BR) {
            printf("\n");
            return;
        }
        plain = (
            node->v.element.tag == GUMBO_TAG_CODE ||
            node->v.element.tag == GUMBO_TAG_PRE
        );
        if (node->v.element.tag == GUMBO_TAG_LI)
            printf("* ");
        children = &node->v.element.children;
        for (i = 0; i < (int) children->length; i++)
            print_tree((GumboNode *) children->data[i], plain);
        if (
            node->v.element.tag == GUMBO_TAG_TITLE ||
            node->v.element.tag == GUMBO_TAG_H1 ||
            node->v.element.tag == GUMBO_TAG_H2 ||
            node->v.element.tag == GUMBO_TAG_H3 ||
            node->v.element.tag == GUMBO_TAG_H4 ||
            node->v.element.tag == GUMBO_TAG_H5 ||
            node->v.element.tag == GUMBO_TAG_H6
        )
            printf("\n\n\n");
        else if (node->v.element.tag == GUMBO_TAG_P)
            printf("\n\n");
        else if (node->v.element.tag == GUMBO_TAG_LI)
            printf("\n");
        else if (node->v.element.tag == GUMBO_TAG_A) {
            attr = gumbo_get_attribute(&node->v.element.attributes, "href");
            if (attr)
                printf(" <%s>", attr->value);
        }
        else if (node->v.element.tag == GUMBO_TAG_IMG) {
            attr = gumbo_get_attribute(&node->v.element.attributes, "alt");
            if (attr && strlen(attr->value))
                printf("\n{image: %s}\n", attr->value);
            else
                printf("\n{image}\n");
        }
    }
}

int
main(int argc, char *argv[])
{
    char *raw_html;
    GumboOutput *parsed_html;
    int length;

    if (argc == 1)
        raw_html = read_stdin();
    else if (argc == 2) {
        FILE *fp = fopen(argv[1], "r");
        read_file(fp, &raw_html, &length);
        fclose(fp);
    } else {
        puts("invalid arguments");
        return 1;
    }
    parsed_html = gumbo_parse(raw_html);
    print_tree(parsed_html->root, 0);
    printf("\n");
    gumbo_destroy_output(&kGumboDefaultOptions, parsed_html);
    free(raw_html);
    return 0;
}
