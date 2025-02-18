#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define DNS_SERVER "8.8.8.8"
#define DNS_PORT 53
#define BUFFER_SIZE 512

// DNS Header structure
struct DNS_HEADER {
    unsigned short id;       // Identification number
    unsigned char rd :1;     // Recursion Desired
    unsigned char tc :1;     // Truncated message
    unsigned char aa :1;     // Authoritative Answer
    unsigned char opcode :4; // Purpose of message
    unsigned char qr :1;     // Query/Response Flag
    unsigned char rcode :4;  // Response code
    unsigned char cd :1;     // Checking Disabled
    unsigned char ad :1;     // Authenticated Data
    unsigned char z :1;      // Reserved
    unsigned char ra :1;     // Recursion Available
    unsigned short q_count;  // Number of question entries
    unsigned short ans_count;// Number of answer entries
    unsigned short auth_count; // Number of authority records
    unsigned short add_count;  // Number of additional records
};

// Query structure
struct QUESTION {
    unsigned short qtype;
    unsigned short qclass;
};

// Convert hostname into DNS format (www.google.com -> 3www6google3com0)
void format_hostname(unsigned char *dns, unsigned char *hostname) {
    int lock = 0, i;
    strcat((char*)hostname, ".");
    for (i = 0; i < strlen((char*)hostname); i++) {
        if (hostname[i] == '.') {
            *dns++ = i - lock;
            for (; lock < i; lock++) {
                *dns++ = hostname[lock];
            }
            lock = i + 1;
        }
    }
    *dns++ = '\0';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <domain>\n", argv[0]);
        return 1;
    }

    unsigned char buffer[BUFFER_SIZE], *qname;
    struct sockaddr_in dest;
    int sockfd, i;
    struct DNS_HEADER *dns = NULL;
    struct QUESTION *qinfo = NULL;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("Socket error");
        return 1;
    }

    dest.sin_family = AF_INET;
    dest.sin_port = htons(DNS_PORT);
    dest.sin_addr.s_addr = inet_addr(DNS_SERVER);

    dns = (struct DNS_HEADER *)&buffer;
    dns->id = (unsigned short) htons(getpid());
    dns->qr = 0;
    dns->opcode = 0;
    dns->aa = 0;
    dns->tc = 0;
    dns->rd = 1; // Recursion Desired
    dns->ra = 0;
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1);
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    qname = (unsigned char*)&buffer[sizeof(struct DNS_HEADER)];
    format_hostname(qname, (unsigned char*)argv[1]);

    qinfo = (struct QUESTION*)&buffer[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)];
    qinfo->qtype = htons(1);
    qinfo->qclass = htons(1);

    if (sendto(sockfd, buffer, sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(struct QUESTION), 0,
               (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        perror("Send failed");
        return 1;
    }

    struct sockaddr_in response_addr;
    socklen_t len = sizeof(response_addr);
    if (recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&response_addr, &len) < 0) {
        perror("Receive failed");
        return 1;
    }

    dns = (struct DNS_HEADER*) buffer;
    unsigned char *reader = &buffer[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1) + sizeof(struct QUESTION)];

    if (ntohs(dns->ans_count) == 0) {
        printf("No answers received.\n");
        return 1;
    }

    printf("\nAnswer Section:\n");
    for (i = 0; i < ntohs(dns->ans_count); i++) {
        reader += 2; // Name pointer
        unsigned short type = ntohs(*(unsigned short*)reader);
        reader += 2;
        reader += 2; // Class
        reader += 4; // TTL
        unsigned short data_len = ntohs(*(unsigned short*)reader);
        reader += 2;

        if (type == 1) { // A record
            printf("IP Address: %d.%d.%d.%d\n", reader[0], reader[1], reader[2], reader[3]);
            reader += data_len;
        } else {
            reader += data_len;
        }
    }

    close(sockfd);
    return 0;
}
