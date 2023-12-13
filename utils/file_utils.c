#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXT_LINE_LENGTH 1024

unsigned char* load_model(const char* filename, int* model_size)
{
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("fopen %s fail!\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    int model_len = ftell(fp);
    unsigned char* model = (unsigned char*)malloc(model_len);
    fseek(fp, 0, SEEK_SET);
    if (model_len != fread(model, 1, model_len, fp)) {
        printf("fread %s fail!\n", filename);
        free(model);
        fclose(fp);
        return NULL;
    }
    *model_size = model_len;
    fclose(fp);
    return model;
}

int read_data_from_file(const char *path, char **out_data)
{
    FILE *fp = fopen(path, "rb");
    if(fp == NULL) {
        printf("fopen %s fail!\n", path);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    char *data = (char *)malloc(file_size+1);
    data[file_size] = 0;
    fseek(fp, 0, SEEK_SET);
    if(file_size != fread(data, 1, file_size, fp)) {
        printf("fread %s fail!\n", path);
        free(data);
        fclose(fp);
        return -1;
    }
    if(fp) {
        fclose(fp);
    }
    *out_data = data;
    return file_size;
}

int write_data_to_file(const char *path, const char *data, unsigned int size)
{
    FILE *fp;

    fp = fopen(path, "w");
    if(fp == NULL) {
        printf("open error: %s\n", path);
        return -1;
    }

    fwrite(data, 1, size, fp);
    fflush(fp);

    fclose(fp);
    return 0;
}

int count_lines(FILE* file)
{
    int count = 0;
    char ch;

    while(!feof(file))
    {
        ch = fgetc(file);
        if(ch == '\n')
        {
            count++;
        }
    }
    count += 1;

    rewind(file);
    return count;
}

char** read_lines_from_file(const char* filename, int* line_count)
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open the file.\n");
        return NULL;
    }

    int num_lines = count_lines(file);
    printf("num_lines=%d\n", num_lines);
    char** lines = (char**)malloc(num_lines * sizeof(char*));
    memset(lines, 0, num_lines * sizeof(char*));

    char buffer[MAX_TEXT_LINE_LENGTH];
    int line_index = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';  // 移除换行符

        lines[line_index] = (char*)malloc(strlen(buffer) + 1);
        strcpy(lines[line_index], buffer);

        line_index++;
    }

    fclose(file);

    *line_count = num_lines;
    return lines;
}

void free_lines(char** lines, int line_count)
{
    for (int i = 0; i < line_count; i++) {
        if (lines[i] != NULL) {
            free(lines[i]);
        }
    }
    free(lines);
}