#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define GOOGLE_DNS "8.8.8.8"  // Google Public DNS
#define GOOGLE_DNS_PORT 53    // DNS Port

// Function to forward DNS query to Google DNS
void forward_to_google(unsigned char *query, int query_len, int client_socket, struct sockaddr_in client_addr) {
    int sock;
    struct sockaddr_in google_dns;
    socklen_t addr_len = sizeof(client_addr);
    unsigned char buffer[512];  // DNS response buffer

    // Create a UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    // Configure Google DNS server details
    memset(&google_dns, 0, sizeof(google_dns));
    google_dns.sin_family = AF_INET;
    google_dns.sin_port = htons(GOOGLE_DNS_PORT);
    google_dns.sin_addr.s_addr = inet_addr(GOOGLE_DNS);

    // Send the query to Google DNS
    sendto(sock, query, query_len, 0, (struct sockaddr*)&google_dns, sizeof(google_dns));

    // Receive response from Google DNS
    int response_len = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
    if (response_len < 0) {
        perror("No response from Google DNS");
        close(sock);
        return;
    }

    // Send the response back to the original client
    sendto(client_socket, buffer, response_len, 0, (struct sockaddr*)&client_addr, addr_len);

    // Close the socket
    close(sock);
}

// Example function to check if a domain is in the local intranet database
int is_intranet_domain(const char *domain) {
    // TODO: Replace with actual database lookup
    if (strcmp(domain, "intranet.local") == 0) {
        return 1;
    }
    return 0;
}

// Function to extract domain name from DNS query
void extract_domain_name(unsigned char *query, char *domain) {
    int i = 12;  // DNS header is 12 bytes long
    int j = 0;

    while (query[i] != 0) {
        int len = query[i];  // Length of the next domain label
        i++;

        // Copy the domain label to the domain buffer
        for (int k = 0; k < len; k++) {
            domain[j++] = query[i++];
        }

        // Add a dot between labels
        domain[j++] = '.';
    }
    domain[j - 1] = '\0';  // Replace last dot with null terminator
}


// Function to handle incoming DNS requests
void handle_dns_query(int sockfd) {
    unsigned char buffer[512];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (1) {
        // Receive DNS request
        int len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &addr_len);
        if (len < 0) {
            perror("Error receiving data");
            continue;
        }

        // Extract the requested domain name
        char domain[256] = {0};
        extract_domain_name(buffer, domain);
        // TODO: Parse the actual domain name from the query packet
        // strcpy(domain, "example.com");  // Placeholder

        printf("Received query for domain: %s\n", domain);

        // Check if the domain is in the intranet
        if (is_intranet_domain(domain)) {
            printf("Domain is in intranet. Responding with local IP...\n");
            // TODO: Return the local IP from database
        } else {
            printf("Domain not found in intranet. Forwarding to Google DNS...\n");
            forward_to_google(buffer, len, sockfd, client_addr);
        }
    }
}

// Main function to set up the DNS server
int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server settings
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(5354);  // Bind to DNS port

    // Bind the socket
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("DNS Server is running on port 5354...\n");

    // Handle DNS queries
    handle_dns_query(sockfd);

    // Close socket (never reached)
    close(sockfd);
    return 0;
}
