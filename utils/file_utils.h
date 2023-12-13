#ifndef _RKNN_MODEL_ZOO_FILE_UTILS_H_
#define _RKNN_MODEL_ZOO_FILE_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read data from file
 * 
 * @param path [in] File path
 * @param out_data [out] Read data
 * @return int -1: error; > 0: Read data size
 */
int read_data_from_file(const char *path, char **out_data);

/**
 * @brief Write data to file
 * 
 * @param path [in] File path
 * @param data [in] Write data
 * @param size [in] Write data size
 * @return int 0: success; -1: error
 */
int write_data_to_file(const char *path, const char *data, unsigned int size);

/**
 * @brief Read all lines from text file
 * 
 * @param path [in] File path
 * @param line_count [out] File line count
 * @return char** String array of all lines, remeber call free_lines() to release after used
 */
char** read_lines_from_file(const char* path, int* line_count);

/**
 * @brief Free lines string array
 * 
 * @param lines [in] String array
 * @param line_count [in] Line count
 */
void free_lines(char** lines, int line_count);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif //_RKNN_MODEL_ZOO_FILE_UTILS_H_