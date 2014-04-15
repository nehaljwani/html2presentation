#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<json/json.h>


#define MAX_TAGS 250
#define MAX_TAG_OCCURENCE 50
#define MAX_H_TAG 5

/* Stupid global pointing to filepath */
char *htmlFile = NULL;

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

json_object *text2json(section *mySection, char *secBuf, section *currTitle) {
    bool isImg = false;
    bool isTable = false;
    char titleBuf[200];
    char textBuf[10000];
    char attrBuf[25];
    char pathBuf[1000];
    char srcBuf[100];
    char realPath[1000];
    bool alt = false;
    size_t i = 0;

    if (mySection->isTag) {
        if (!strcmp(mySection->tag, "img"))
            isImg = true;
        else
            return NULL;
    }

    /* Do the dew for images */
    if (isImg) {
        char *src_begin = strchr(mySection->content, '"');
        char *src_end = strchr(src_begin + 1, '"');
        char *alt_begin = mySection->content;
        for (i = 0; i < mySection->length; i++) {
            if (!strncmp(alt_begin, "alt", 3)) {
                alt_begin += 5;
                alt = true;             /*alt="*/
                break;
            }
            alt_begin++;
        }
        char *alt_end = strchr(alt_begin + 1, '"');
        snprintf(titleBuf, currTitle->length + 1, "%s", currTitle->content);
        snprintf(textBuf, alt_end - alt_begin + 1, "%s", alt_begin);
        sprintf(attrBuf, "image");
        char *path_begin = htmlFile;
        char *path_end = strrchr(htmlFile, '/');
        snprintf(pathBuf, path_end - path_begin + 2, "%s", path_begin);
        snprintf(srcBuf, src_end - src_begin, "%s", src_begin + 1);
        strcat(pathBuf, srcBuf);
        realpath(pathBuf, realPath);
    }
    /* Do the dew for normal */
    else {
        snprintf(titleBuf, currTitle->length + 1, "%s", currTitle->content);
        snprintf(textBuf, mySection->length + 1, "%s", mySection->content);
    }

    json_object *jobj = json_object_new_object();
    json_object *jstring1 = json_object_new_string(titleBuf);
    json_object_object_add(jobj,"title", jstring1);
    if (mySection->isTag && alt || !mySection->tag) {
        json_object *jstring2 = json_object_new_string(textBuf);
        json_object_object_add(jobj,"text", jstring2);
    }
    json_object *jstring3 = json_object_new_string(secBuf);
    json_object_object_add(jobj,"section", jstring3);
    if (isImg) {
        json_object *jstring4 = json_object_new_string("img");
        json_object_object_add(jobj,"attr", jstring4);
        json_object *jstring5 = json_object_new_string(realPath);
        json_object_object_add(jobj,"path", jstring5);
    }

    return jobj;
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
    char secBuf[150];

    for (i = 0; i <= MAX_H_TAG; i++)
        headers[i] = 0;

    section *mySection = tmpStore->ptrs[0];
    section *currTitle = NULL;
    json_object *jarray = json_object_new_array();

    while (mySection->next != NULL) {
        if (mySection->isTag) {
            if (strlen(mySection->tag) == 2
                && (mySection->tag[0] == 'H'
                    || mySection->tag[0] == 'h')) {
                int tmpTag = atoi(&mySection->tag[1]);
                currTitle = mySection->next;
                if (tmpTag) {
                    if (tmpTag < currTag) {
                        for (i = tmpTag + 1; i <= currTag; i++)
                            headers[i] = 0;
                    }
                    currTag = tmpTag;
                    headers[currTag]++;
                    sprintf(secBuf, "Section :1");
                    for (i = 2; i <= currTag; i++)
                        sprintf(secBuf + strlen(secBuf), ".%d", headers[i]);
                }
            }
        }
        json_object *jobj = text2json(mySection, secBuf, currTitle);
        if (jobj)
            json_object_array_add(jarray, jobj);
        mySection = mySection->next;
    }
    printf ("%s\n", json_object_to_json_string_ext(jarray, JSON_C_TO_STRING_PRETTY));
}

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage %s <html_file>\n", argv[0]);
        exit(-1);
    }

    FILE *fp = fopen(argv[1], "r");
    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    /* Since we have the file size, lets read the complete file in memory */
    char *fileContent = readFile(fp, size);
    fclose(fp);

    htmlFile = argv[1];

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
