#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define MAX_TAGS 250
#define MAX_TAG_OCCURENCE 50
#define MAX_H_TAG 5

/* bool is not available in C */
typedef enum { false, true } bool;

typedef struct _section section;
struct _section {
    bool isTag;         /* Is this section a tag? */
    char *tag;          /* If tag, fill this */
    char *content;      /* Pointer to content of this section */
    int length;         /* Length of the content */
    section *next;      /* Pinter to the next section */
};

typedef struct _ptrStore ptrStore;
struct _ptrStore {
    section *ptrs[MAX_TAG_OCCURENCE]; /* Array of pointers */
    int count;                        /* Number of elements in array */
};

unsigned int m_size = MAX_TAGS;
/*
 * djb2:
 * @key: key to be hashed
 *
 * Returns the hash of the key using djb2 algorithm
 */
unsigned int djb2(const char* key) {
    unsigned int i, hash = 5381;

    for (i = 0; i < strlen(key); i++)
        hash = ((hash << 5) + hash) + (unsigned int)tolower(key[i]);

    return hash % m_size;
}

/*
 * readFile:
 * @fp: pointer to file to be read
 * @size: size of the file
 *
 * Returns pointer to the in memory buffer
 * which holds the entire file contents
 */
char *readFile(FILE *fp, long size) {
    int c;
    size_t n = 0;
    if (fp == NULL)
        return NULL;
    char *code = (char *)malloc(size * sizeof(char));
    while ((c = fgetc(fp)) != EOF)
        code[n++] = (char) c;
    code[n] = '\0';
    return code;
}

/*
 * printHeaders:
 *
 * @myPtrStore: pointer to pointer store
 *
 * Prints the entire document along with appropriate sections
 * If input is:
 * <H1> Blah </H1>
 * <H2> Blah </H2>
 * <H3> Blah </H3>
 * <H2> Blah </H2>
 * <H1> Blah </H1>
 *
 * Then the output will be:
 * Section 1 <Blah>
 * Section 1.2 <Blah>
 * Section 1.2.1 <Blah>
 * Section 1.2.2 <Blah>
 * Section 1.3 <Blah>
 */
void printHeaders(ptrStore *myPtrStore) {
    int headers[MAX_H_TAG + 1];
    ptrStore *tmpStore = &myPtrStore[djb2("H1")];
    int currTag = 1;
    size_t i = 0;

    for (i = 0; i <= MAX_H_TAG; i++)
        headers[i] = 0;

    section *mySection = tmpStore->ptrs[0];
    while (mySection->next != NULL) {
        if (mySection->isTag) {
            if (strlen(mySection->tag) == 2
                && (mySection->tag[0] == 'H'
                    || mySection->tag[0] == 'h')) {
                int tmpTag = atoi(&mySection->tag[1]);
                if (tmpTag) {
                    if (tmpTag < currTag) {
                        for (i = tmpTag + 1; i <= currTag; i++)
                            headers[i] = 0;
                    }
                    currTag = tmpTag;
                    headers[currTag]++;
                    printf("\nSection :1");
                    for (i = 2; i <= currTag; i++)
                        printf(".%d", headers[i]);
                    printf("\n");
                }
            }
        }
        else {
            fflush(stdout);
            write(fileno(stdout), mySection->content, mySection->length);
        }
        mySection = mySection->next;
    }
}

int main(int argc, char **argv) {
    FILE *fp = fopen(argv[1], "r");
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    /* Since we have the file size, lets read the complete file in memory */
    char *fileContent = readFile(fp, size);
    fclose(fp);

    size_t i;
    char *parser = fileContent;
    section *mySections = (section *)malloc(sizeof(section));
    section *curr = mySections;
    int traversal = 0;
    ptrStore *myPtrStore = (ptrStore *)malloc(sizeof(ptrStore) * MAX_TAGS);
    for (i = 0; i < MAX_TAGS; i++)
        myPtrStore[i].count = 0;

    /* Parse the whole file */
    while (traversal <= size) {
        curr->next = (section *)malloc(sizeof(section));
        curr->next->content = parser;
        curr->next->length = 0;
        curr->next->next = NULL;
        int flag = 0;
        char *tag = NULL;
        /* Start from <? to > */
        if (*parser == '<' && *(parser + 1) != ' ') {
            curr->next->isTag = true;
            while (*(++parser) != '>') {
                if (!flag && (*parser == ' ' || *(parser + 1) == '>')) {
                    if (*parser == ' ')
                        tag = strndup(curr->next->content + 1, curr->next->length);
                    if (*(parser + 1)== '>')
                        tag = strndup(curr->next->content + 1, curr->next->length + 1);
                    ptrStore *tmpStore = &myPtrStore[djb2(tag)];
                    tmpStore->ptrs[tmpStore->count++] = curr->next;
                    flag = !flag;
                    curr->next->tag = tag;
                }
                curr->next->length++;
                traversal++;
            }
            curr->next->length += 2;
            parser++;
            traversal++;
        }
        /* Start from character after > till start of next tag */
        else {
            curr->next->isTag = false;
            while (!(*parser == '<' && *(parser + 1) != ' ') && traversal <= size) {
                curr->next->length++;
                traversal++;
                parser++;
            }
        }
        curr = curr->next;
    }

    /* Just to make sure we got the contents right *//*
    section *print = mySections->next;
    while (print->next != NULL) {
        printf("\nTag: %s\n", print->isTag ? "Yes" : "No");
        fflush(stdout);
        write(fileno(stdout), print->content, print->length);
        print = print->next;
    }*/

    /* Just to make sure hashed values point to right things correct *//*
    ptrStore *tmp = &myPtrStore[djb2("HTML")];
    printf("%d\n", tmp->count);*/

    printHeaders(myPtrStore);

    free(myPtrStore);
    free(mySections);
    free(fileContent);
    return 0;
}
