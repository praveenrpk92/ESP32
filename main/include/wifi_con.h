#ifndef WIFI_CON_H
#define WIFI_CON_H


#define MAX_USSID_SIZE 32
#define MAX_UPWD_SIZE 50

struct wifi_credentials
    {
        unsigned char ussid[MAX_USSID_SIZE];
        unsigned char upwd[MAX_UPWD_SIZE];
    };

void wifi_main(struct wifi_credentials *);
void wifi_connect(struct wifi_credentials *);
#endif
