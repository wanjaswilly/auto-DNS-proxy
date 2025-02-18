#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define GOOGLE_PUBLIC_DNS_IP "8.8.8.8"  // Google Public DNS IP
#define GOOGLE_DNS_PORT 53              // Google DNS Port

// Function to forward the DNS query to Google DNS server and get the response
void forward_dns_query_to_google(unsigned char *dns_query, int dns_query_length, int client_socket_fd, struct sockaddr_in client_address) {
    int google_dns_socket_fd;
    struct sockaddr_in google_dns_server_address;
    socklen_t client_address_length = sizeof(client_address);
    unsigned char dns_response_buffer[512];  // Buffer to store DNS response

    // Create a UDP socket to communicate with Google DNS
    if ((google_dns_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating socket for communication with Google DNS");
        return;
    }

    // Set up the Google DNS server details (IP and Port)
    memset(&google_dns_server_address, 0, sizeof(google_dns_server_address));
    google_dns_server_address.sin_family = AF_INET;
    google_dns_server_address.sin_port = htons(GOOGLE_DNS_PORT);
    google_dns_server_address.sin_addr.s_addr = inet_addr(GOOGLE_PUBLIC_DNS_IP);

    // Send the DNS query to Google DNS server
    sendto(google_dns_socket_fd, dns_query, dns_query_length, 0, (struct sockaddr*)&google_dns_server_address, sizeof(google_dns_server_address));

    // Receive the DNS response from Google DNS
    int dns_response_length = recvfrom(google_dns_socket_fd, dns_response_buffer, sizeof(dns_response_buffer), 0, NULL, NULL);
    if (dns_response_length < 0) {
        perror("Error receiving response from Google DNS");
        close(google_dns_socket_fd);
        return;
    }

    // Forward the DNS response back to the original client
    sendto(client_socket_fd, dns_response_buffer, dns_response_length, 0, (struct sockaddr*)&client_address, client_address_length);

    // Close the socket after the communication
    close(google_dns_socket_fd);
}

// Function to check if the domain is part of the local intranet
int check_if_domain_is_in_intranet(const char *domain_name) {
    // TODO: Replace with actual database lookup for intranet domain
    if (strcmp(domain_name, "intranet.local") == 0) {
        return 1;  // Intranet domain found
    }
    return 0;  // Domain not found in intranet
}

// Function to extract the domain name from the DNS query packet
void extract_domain_name_from_dns_query(unsigned char *dns_query, char *domain_name) {
    int i = 12;  // DNS header is always 12 bytes long
    int j = 0;

    // Loop through the DNS query to extract each label of the domain
    while (dns_query[i] != 0) {
        int label_length = dns_query[i];  // Length of the next domain label
        i++;

        // Copy the domain label to the domain_name buffer
        for (int k = 0; k < label_length; k++) {
            domain_name[j++] = dns_query[i++];
        }

        // Add a dot between domain labels
        domain_name[j++] = '.';
    }
    domain_name[j - 1] = '\0';  // Replace the last dot with a null terminator
}

// Function to handle incoming DNS queries from clients
void handle_dns_queries_on_socket(int server_socket_fd) {
    unsigned char dns_query_buffer[512];
    struct sockaddr_in client_address;
    socklen_t client_address_length = sizeof(client_address);

    while (1) {
        // Receive DNS request from client
        int dns_query_length = recvfrom(server_socket_fd, dns_query_buffer, sizeof(dns_query_buffer), 0, (struct sockaddr*)&client_address, &client_address_length);
        if (dns_query_length < 0) {
            perror("Error receiving DNS query from client");
            continue;
        }

        // Extract the requested domain name from the DNS query
        char extracted_domain_name[256] = {0};
        extract_domain_name_from_dns_query(dns_query_buffer, extracted_domain_name);

        // Log the domain name for debugging
        printf("Received DNS query for domain: %s\n", extracted_domain_name);

        // Check if the domain is part of the intranet
        if (check_if_domain_is_in_intranet(extracted_domain_name)) {
            printf("Domain is found in the intranet. Responding with local IP...\n");
            // TODO: Send the local intranet IP response here (from database)
        } else {
            printf("Domain not found in the intranet. Forwarding the query to Google DNS...\n");
            forward_dns_query_to_google(dns_query_buffer, dns_query_length, server_socket_fd, client_address);
        }
    }
}

// Main function to initialize and run the DNS server
int main() {
    int dns_server_socket_fd;
    struct sockaddr_in dns_server_address;

    // Create a UDP socket for the DNS server
    if ((dns_server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating DNS server socket");
        exit(EXIT_FAILURE);
    }

    // Set up the DNS server address (bind to port 5354)
    memset(&dns_server_address, 0, sizeof(dns_server_address));
    dns_server_address.sin_family = AF_INET;
    dns_server_address.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces
    dns_server_address.sin_port = htons(5354);  // DNS server listening on port 5354

    // Bind the socket to the DNS server address
    if (bind(dns_server_socket_fd, (struct sockaddr*)&dns_server_address, sizeof(dns_server_address)) < 0) {
        perror("Error binding DNS server socket");
        exit(EXIT_FAILURE);
    }

    // Log that the DNS server is up and running
    printf("DNS Server is running on port 5354...\n");

    // Start handling DNS queries
    handle_dns_queries_on_socket(dns_server_socket_fd);

    // Close the server socket (never reached in this example)
    close(dns_server_socket_fd);
    return 0;
}
