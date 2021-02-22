#ifndef PROJECT_INCLUDE_IP_PARSER_H_
#define PROJECT_INCLUDE_IP_PARSER_H_

int dec_to_bin (int num);
int bin_to_dec(int num);

int get_octs_and_mask_from_ip(const char* get_ip, int octs[4], int* mask);
int get_binary_network_addr_from_octs(const int octs[4], const int mask, char* binary_network_addr);
int get_binary_octs_str_from_binary_network_addr(const char* binary_network_addr, char binary_octs[4][255]);
int get_network_addr_from_binary_octs(const char binary_octs[4][255], char* network_addr);

int get_network_and_mask(const char* get_ip, char* network_addr, int* mask);

int is_in_network(const char* network_addr, const int net_mask,  const char* ip_addr);

#endif  // PROJECT_INCLUDE_PARSER_H_