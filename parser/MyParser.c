#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<ctype.h>
#include<unistd.h>

#include<json/json.h>

#define MAX_TAGS 11000
#define MAX_TAG_OCCURENCE 1000
#define MAX_H_TAG 1000

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
unsigned int djb2(const char* key)
{
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
char *
readFile(FILE *fp,
         long size)
{
    int c;
    size_t n = 0;
    if (fp == NULL)
        return NULL;
    char *code = (char *)malloc(size * sizeof(char));
    while ((c = fgetc(fp)) != EOF) {
        if ((char)c != '\r' && (char)c != '\n' && c < 128)
            code[n++] = (char) c;
    }
    code[n] = '\0';
    return code;
}

/*
 * text2json:
 * @mySection: pointer to file sections
 * @secBuf: string holdingc current section number
 * @curTitle: pointer to the current section title
 *
 * Returns a json formatted object of the content
 */
json_object *
text2json(section *mySection,
          char *secBuf,
          section *currTitle)
{
    bool isImg = false;
    char titleBuf[1000];
    char textBuf[20000];
    char attrBuf[2500];
    char pathBuf[2000];
    char srcBuf[1000];
    char realPath[2000];
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
    if ((mySection->isTag && alt) || !mySection->tag) {
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

char *createDaddy(char *str, long *osize, char *files, char *basedir) {
    char *pch = str;
    char path[320];
    char realPath[320];
    char newPath[320];
    while (*pch) {
        if (!strncasecmp("href", pch, 4)) {
            char *begin = strchr(pch, '"');
            char *end = strchr(begin + 1, '"');
            snprintf(path, end - begin, "%s\n", begin + 1);
            path[end - begin] = '\0';
            end = strchr(path, '#');
            if (end != NULL)
                *end = '\0';
            if (path[0] != '#'
                && strncasecmp(path, "http", 4)
                && strncasecmp(path, "ftp", 3)
                && strncasecmp(path, "../../", 6)
                && strncasecmp(path, "./../../", 7)
                && (!strncasecmp(path + strlen(path) - 4, ".htm", 4)
                    || !strncasecmp(path + strlen(path) - 5, ".html", 5))) {
                if(!files[djb2(path) % 100]) {
                    files[djb2(path) % 100] = 1;
                    char *tmp = strrchr(basedir, '/');
                    *tmp = '\0';
                    sprintf(newPath, "%s/%s", basedir, path);
                    *tmp = '/';
                    realpath(newPath, realPath);
                    FILE *fp = fopen(realPath, "r");
                    if (!fp)
                        continue;
                    fseek(fp, 0L, SEEK_END);
                    long size = ftell(fp);
                    fseek(fp, 0L, SEEK_SET);

                    /* Since we have the file size, lets read the complete file in memory */
                    char *fileContent = readFile(fp, size);
                    fclose(fp);
                    while(*pch != '>') pch++;
                    char *newStr = (char *) malloc(sizeof(char) * size + *osize);
                    snprintf(newStr, (long)pch - (long)str, "%s", str);
                    strcat(newStr, fileContent);
                    strcat(newStr, pch + 1);
                    free(fileContent);
                    free(str);
                    *osize += size;
                    pch = newStr + (long)pch - (long)str;
                    str = newStr;
                }
            }
        }
        pch++;
    }
    return str;
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
 * Section 1.3 <Blah> .... json format :)
 */
void
printHeaders(ptrStore *myPtrStore, section *mySection)
{
    int headers[MAX_H_TAG + 1];
    ptrStore *tmpStore = &myPtrStore[djb2("title")];
    int currTag = 1;
    size_t i = 0;
    char secBuf[150];
    sprintf(secBuf, "Section: 0");

    for (i = 0; i <= MAX_H_TAG; i++)
        headers[i] = 0;

    if (tmpStore->ptrs[0])
        mySection = tmpStore->ptrs[0];
    section *currTitle = mySection->next;
    json_object *jarray = json_object_new_array();
    json_object *jobj;

    while (mySection->next != NULL) {
        if (mySection->isTag) {
            if (strlen(mySection->tag) == 2
                && (mySection->tag[0] == 'H'
                    || mySection->tag[0] == 'h')) {
                int tmpTag = atoi(&mySection->tag[1]);
                if (tmpTag) {
                    section *tmpSection = mySection;
                    while(tmpSection->isTag)
                        tmpSection = tmpSection->next;
                    currTitle = tmpSection;

                    if (tmpTag < currTag) {
                        for (i = tmpTag + 1; i <= currTag; i++)
                            headers[i] = 0;
                    }
                    currTag = tmpTag;
                    headers[currTag]++;
                    sprintf(secBuf, "Section: 1");
                    for (i = 2; i <= currTag; i++)
                        sprintf(secBuf + strlen(secBuf), ".%d", headers[i]);
                }
            }
            if (!strcmp(mySection->tag, "table")) {
                section *tmpSection = mySection->next;
                json_object *jarray_rows = json_object_new_array();
                while(1) {
                    if (tmpSection->isTag) {
                        if (!tmpSection)
                            goto safeHouse;
                        if (!strcmp(tmpSection->tag, "tr")) {
                            json_object *jarray_cols = json_object_new_array();
                            while (1) {
                                if (!tmpSection)
                                    goto safeHouse;
                                if (tmpSection->isTag) {
                                    json_object *jarray_cell = json_object_new_array();
                                    if (!strcmp(tmpSection->tag, "td")
                                        || !strcmp(tmpSection->tag, "th")) {
                                        while(1) {
                                            if (!tmpSection)
                                                goto safeHouse;
                                            if ((tmpSection->isTag
                                                 && (!strcmp(tmpSection->tag, "/td")
                                                     || !strcmp(tmpSection->tag, "/th")))
                                                || ((tmpSection->next->isTag)
                                                    && (!strcmp(tmpSection->next->tag, "tr")
                                                        || !strcmp(tmpSection->next->tag, "/table")))) {
                                                json_object_array_add(jarray_cols, jarray_cell);
                                                break;
                                            }
                                            jobj = text2json(tmpSection, secBuf, currTitle);
                                            if (jobj) {
                                                json_object_array_add(jarray_cell, jobj);
                                            }
                                            tmpSection = tmpSection->next;
                                        }
                                    }
                                    if (tmpSection->isTag
                                        && (!strcmp(tmpSection->tag, "/tr")
                                            || !strcmp(tmpSection->tag, "/table"))) {
                                        break;
                                    }
                                }
                                tmpSection = tmpSection->next;
                            }
                            if (jarray_cols)
                                json_object_array_add(jarray_rows, jarray_cols);
                        }
                        if (!strcmp(tmpSection->tag, "/table"))
                            break;
                    }
                    tmpSection = tmpSection->next;
                }
                if (jarray_rows) {
                    char titleBuf[255];
                    jobj = json_object_new_object();
                    json_object_object_add(jobj, "table", jarray_rows);
                    json_object *jsection = json_object_new_string(secBuf);
                    json_object_object_add(jobj, "section", jsection);
                    snprintf(titleBuf, currTitle->length + 1, "%s", currTitle->content);
                    json_object *jtitle = json_object_new_string(titleBuf);
                    json_object_object_add(jobj, "title", jtitle);
                    json_object_array_add(jarray, jobj);
                }
                mySection = tmpSection;
            }
        }
safeHouse:
        jobj = text2json(mySection, secBuf, currTitle);
        if (jobj)
            json_object_array_add(jarray, jobj);
        mySection = mySection->next;
    }
    printf ("%s\n", json_object_to_json_string_ext(jarray, JSON_C_TO_STRING_PRETTY));
}

int main(int argc, char **argv)
{

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

    char refFiles[100];
    memset(refFiles, 0, 100);
    fileContent = createDaddy(fileContent, &size, refFiles, argv[1]);

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
    while (traversal <= size && *parser) {
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
                    for (i = 0; i <= curr->next->length; i++)
                        tag[i] = tolower(tag[i]);
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

    /* Just to make sure we got the contents right
    section *print = mySections->next;
    while (print->next != NULL) {
        printf("\nTag: %s\n", print->isTag ? "Yes" : "No");
        fflush(stdout);
        write(fileno(stdout), print->content, print->length);
        print = print->next;
    }*/

    /* Just to make sure hashed values point to right things correct
       ptrStore *tmp = &myPtrStore[djb2("HTML")];
       printf("%d\n", tmp->count);*/

    printHeaders(myPtrStore, mySections);

    free(myPtrStore);
    free(mySections);
    return 0;
}
