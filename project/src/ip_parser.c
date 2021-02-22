#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ip_parser.h"
#include "utils.h"

int dec_to_bin (int num) {
    int bin = 0, k = 1;

    while (num) {
        bin += (num % 2) * k;
        k *= 10;
        num /= 2;
    }

    return bin;
}

int bin_to_dec(int num) {
    int dec = 0, i = 0, rem;
    while (num != 0) {
        rem = num % 10;
        num /= 10;
        dec += rem * (int)pow(2.0, (double)i);
        ++i;
    }
    return dec;
}

int get_octs_and_mask_from_ip(const char* get_ip, int octs[4], int* mask) {
    sscanf(get_ip, "%d.%d.%d.%d/%d", &octs[0], &octs[1], &octs[2], &octs[3], mask);
    return SUCCESS;
}

int get_binary_network_addr_from_octs(const int octs[4], const int mask, char* binary_network_addr) {
    char binary_network_ip[255] = {};
    sprintf(binary_network_ip, "%08d%08d%08d%08d", dec_to_bin(octs[0]), 
                                                   dec_to_bin(octs[1]), 
                                                   dec_to_bin(octs[2]), 
                                                   dec_to_bin(octs[3]));
    strncpy(binary_network_addr, binary_network_ip, mask);
    return SUCCESS;
}

int get_binary_octs_str_from_binary_network_addr(const char* binary_network_addr, char binary_octs[4][255]) {
    for (int i = 0; i < 4; i++) {
        strncpy(binary_octs[i], binary_network_addr + i * 8, 8);
    }

    return SUCCESS;
}

int get_network_addr_from_binary_octs(const char binary_octs[4][255], char* network_addr) {
    sprintf(network_addr, "%d.%d.%d.%d", bin_to_dec(atoi(binary_octs[0])), 
                                          bin_to_dec(atoi(binary_octs[1])), 
                                          bin_to_dec(atoi(binary_octs[2])), 
                                          bin_to_dec(atoi(binary_octs[3])));

    return SUCCESS;
}

int get_network_and_mask(const char* get_ip, char* network_addr, int* mask) {
    int octs[4] = {};
    get_octs_and_mask_from_ip(get_ip, octs, mask);

    char binary_network_addr[255] = {};
    get_binary_network_addr_from_octs(octs, *mask, binary_network_addr);

    char binary_octs[4][255] = {};
    get_binary_octs_str_from_binary_network_addr(binary_network_addr, binary_octs);

    get_network_addr_from_binary_octs(binary_octs, network_addr);
    return SUCCESS;
}



int is_in_network(const char* network_addr, const int net_mask, const char* ip_addr) {
    char lower_addr[255] = {};
    strncpy(lower_addr, network_addr, strlen(network_addr));
    lower_addr[net_mask] = '1';

    char upper_addr[255] = {};
    strncpy(upper_addr, network_addr, strlen(network_addr));
    for (int i = net_mask; i < 31; i++) {
        upper_addr[i] = '1';
    }

    if (strncmp(ip_addr, lower_addr, strlen(ip_addr)) >= 0 && strncmp(ip_addr, upper_addr, strlen(ip_addr)) <= 0) {
        return SUCCESS;
    }

    return FAILURE;
}
