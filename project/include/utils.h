#ifndef _PROJECT_INCLUDE_UTILS_H_
#define _PROJECT_INCLUDE_UTILS_H_

#define FAILURE -1
#define SUCCESS 0

int send_all(int sock, char* str, int len);
int recv_all(int sock, int len , char* result_str);

#endif  // _PROJECT_INCLUDE_UTILS_H_
