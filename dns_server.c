#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sqlite3.h>
#include <regex.h>

// Define constants for Google's public DNS
#define GOOGLE_PUBLIC_DNS "8.8.8.8"
#define GOOGLE_DNS_PORT 53

// Function to execute a SQL query and handle any potential errors
int execute_sql_query(sqlite3 *database_connection, const char *sql_query)
{
    char *error_message = 0;
    int result_code = sqlite3_exec(database_connection, sql_query, 0, 0, &error_message);
    if (result_code != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", error_message);
        sqlite3_free(error_message);
        return result_code;
    }
    return SQLITE_OK;
}

// Function to save DNS query details (domain, query time, server IP, port) into the database
int save_dns_query_to_database(sqlite3 *database_connection, const char *domain_name, int query_time, const char *server_ip, int port_number)
{
    sqlite3_stmt *statement;
    const char *sql_insert_query = "INSERT INTO dns_queries (domain, query_time, server_ip, port) VALUES (?, ?, ?, ?);";
    int result_code = sqlite3_prepare_v2(database_connection, sql_insert_query, -1, &statement, NULL);
    if (result_code != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(database_connection));
        return result_code;
    }

    // Bind values to the SQL statement
    sqlite3_bind_text(statement, 1, domain_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(statement, 2, query_time);
    sqlite3_bind_text(statement, 3, server_ip, -1, SQLITE_STATIC);
    sqlite3_bind_int(statement, 4, port_number);

    // Execute the statement to insert the DNS query details into the database
    result_code = sqlite3_step(statement);
    if (result_code != SQLITE_DONE)
    {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(database_connection));
        sqlite3_finalize(statement);
        return result_code;
    }

    sqlite3_finalize(statement);
    return SQLITE_OK;
}

// Function to save the DNS answer details (IP addresses and TTL) to the database
int save_dns_answer_to_database(sqlite3 *database_connection, int query_id, const char *ip_address, int ttl)
{
    sqlite3_stmt *statement;
    const char *sql_insert_answer_query = "INSERT INTO dns_answers (query_id, ip_address, ttl) VALUES (?, ?, ?);";
    int result_code = sqlite3_prepare_v2(database_connection, sql_insert_answer_query, -1, &statement, NULL);
    if (result_code != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(database_connection));
        return result_code;
    }

    // Bind values to the SQL statement
    sqlite3_bind_int(statement, 1, query_id);
    sqlite3_bind_text(statement, 2, ip_address, -1, SQLITE_STATIC);
    sqlite3_bind_int(statement, 3, ttl);

    // Execute the statement to insert the DNS answer details into the database
    result_code = sqlite3_step(statement);
    if (result_code != SQLITE_DONE)
    {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(database_connection));
        sqlite3_finalize(statement);
        return result_code;
    }

    sqlite3_finalize(statement);
    return SQLITE_OK;
}

// Function to parse the DNS query response and save it to the database
void parse_dns_response_and_save_to_database(const char *dig_response)
{
    sqlite3 *database_connection;
    int result_code = sqlite3_open("dns_records.db", &database_connection);
    if (result_code != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(database_connection));
        return;
    }

    // Regex patterns to extract domain, IP addresses, and TTL from the response
    regex_t regex_for_domain, regex_for_answer;
    regcomp(&regex_for_domain, "QUESTION SECTION:\n;([^ ]+)", REG_EXTENDED);
    regcomp(&regex_for_answer, "ANSWER SECTION:\n([^ ]+)\\s+([0-9]+)\\s+IN\\s+A\\s+([0-9\\.]+)", REG_EXTENDED);

    // Temporary variables to hold parsed data
    char domain_name[256] = {0};
    char dns_server_ip[16] = "127.0.0.1";  // Placeholder for server IP
    int dns_query_time = 72;                // Placeholder for query time
    int dns_query_port = 5354;              // Placeholder for port number

    // Match the domain name from the QUESTION SECTION
    regmatch_t domain_match_results[2];
    if (regexec(&regex_for_domain, dig_response, 2, domain_match_results, 0) == 0)
    {
        strncpy(domain_name, dig_response + domain_match_results[1].rm_so, domain_match_results[1].rm_eo - domain_match_results[1].rm_so);
    }

    // Save the DNS query details to the database
    int query_result_code = save_dns_query_to_database(database_connection, domain_name, dns_query_time, dns_server_ip, dns_query_port);
    if (query_result_code != SQLITE_OK)
    {
        sqlite3_close(database_connection);
        return;
    }

    // Get the query ID of the saved query
    int query_id = sqlite3_last_insert_rowid(database_connection);

    // Match the answers (IP addresses and TTL values)
    regmatch_t answer_match_results[4];
    const char *answer_section_start = strstr(dig_response, "ANSWER SECTION:");
    if (answer_section_start)
    {
        const char *line_start = answer_section_start;
        while (regexec(&regex_for_answer, line_start, 4, answer_match_results, 0) == 0)
        {
            char ip_address[16];
            int ttl_value;

            // Extract IP address and TTL value
            strncpy(ip_address, line_start + answer_match_results[3].rm_so, answer_match_results[3].rm_eo - answer_match_results[3].rm_so);
            ttl_value = atoi(line_start + answer_match_results[2].rm_so);

            // Save the DNS answer details to the database
            save_dns_answer_to_database(database_connection, query_id, ip_address, ttl_value);

            // Move to the next line
            line_start += answer_match_results[0].rm_eo;
        }
    }

    // Free the compiled regex patterns and close the database connection
    regfree(&regex_for_domain);
    regfree(&regex_for_answer);
    sqlite3_close(database_connection);
}

// Function to forward DNS query to Google's public DNS server and receive the response
void forward_dns_query_to_google_dns(unsigned char *query_data, int query_length, int client_socket, struct sockaddr_in client_address)
{
    int socket_fd;
    struct sockaddr_in google_dns_server;
    socklen_t address_length = sizeof(client_address);
    unsigned char response_buffer[512]; // DNS response buffer

    // Create a UDP socket
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return;
    }

    // Set up Google's DNS server details
    memset(&google_dns_server, 0, sizeof(google_dns_server));
    google_dns_server.sin_family = AF_INET;
    google_dns_server.sin_port = htons(GOOGLE_DNS_PORT);
    google_dns_server.sin_addr.s_addr = inet_addr(GOOGLE_PUBLIC_DNS);

    // Send the DNS query to Google's DNS server
    sendto(socket_fd, query_data, query_length, 0, (struct sockaddr *)&google_dns_server, sizeof(google_dns_server));

    // Receive the response from Google DNS
    int response_length = recvfrom(socket_fd, response_buffer, sizeof(response_buffer), 0, NULL, NULL);
    if (response_length < 0)
    {
        perror("No response from Google DNS");
        close(socket_fd);
        return;
    }

    // Save the DNS response to the database
    parse_dns_response_and_save_to_database((char *)response_buffer);

    // Send the response back to the original client
    sendto(client_socket, response_buffer, response_length, 0, (struct sockaddr *)&client_address, address_length);

    // Close the socket
    close(socket_fd);
}

// Example function to check if a domain is part of the intranet
int is_domain_part_of_intranet(const char *domain_name)
{
    sqlite3 *database_connection;
    sqlite3_stmt *statement;
    int result_code;
    const char *database_file = "dns_records.db";  // Database file

    // Check if the domain is part of the intranet (you can customize this logic)
    if (strcmp(domain_name, "intranet.local") == 0)
    {
        return 1;
    }

    // Open the SQLite database
    result_code = sqlite3_open(database_file, &database_connection);
    if (result_code)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(database_connection));
        return 0;
    }

    // Prepare SQL query to check if the domain exists in the DNS queries table
    const char *sql_select_query = "SELECT id FROM dns_queries WHERE domain = ?";
    result_code = sqlite3_prepare_v2(database_connection, sql_select_query, -1, &statement, NULL);
    if (result_code != SQLITE_OK)
    {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(database_connection));
        sqlite3_close(database_connection);
        return 0;
    }

    // Bind the domain parameter to the SQL query
    sqlite3_bind_text(statement, 1, domain_name, -1, SQLITE_STATIC);

    // Execute the query and check if the domain exists in the DNS queries table
    result_code = sqlite3_step(statement);
    if (result_code == SQLITE_ROW)
    {
        printf("Domain %s found in DNS queries table.\n", domain_name);
        sqlite3_finalize(statement);
        sqlite3_close(database_connection);
        return 1; // Domain found in DNS queries
    }

    // Domain not found in the DNS queries table
    printf("Domain %s not found in DNS queries table.\n", domain_name);
    sqlite3_finalize(statement);
    sqlite3_close(database_connection);
    return 0; // Domain not found in DNS queries
}

// Function to extract the domain name from the DNS query
void extract_domain_name_from_query(unsigned char *query_data, char *domain_name)
{
    int index = 12; // DNS header is 12 bytes long
    int domain_index = 0;

    while (query_data[index] != 0)
    {
        int label_length = query_data[index];  // Length of the next domain label
        index++;

        // Copy the domain label to the domain buffer
        for (int i = 0; i < label_length; i++)
        {
            domain_name[domain_index++] = query_data[index++];
        }

        // Add a dot between labels
        domain_name[domain_index++] = '.';
    }
    domain_name[domain_index - 1] = '\0'; // Replace the last dot with a null terminator
}

// Function to handle incoming DNS queries
void handle_incoming_dns_queries(int socket_fd)
{
    unsigned char query_buffer[512];
    struct sockaddr_in client_address;
    socklen_t address_length = sizeof(client_address);

    while (1)
    {
        // Receive DNS query
        int query_length = recvfrom(socket_fd, query_buffer, sizeof(query_buffer), 0, (struct sockaddr *)&client_address, &address_length);
        if (query_length < 0)
        {
            perror("Error receiving data");
            continue;
        }

        // Extract the domain name from the query
        char domain_name[256] = {0};
        extract_domain_name_from_query(query_buffer, domain_name);

        printf("Received query for domain: %s\n", domain_name);

        // Check if the domain is part of the intranet
        if (is_domain_part_of_intranet(domain_name))
        {
            printf("Domain is part of the intranet. Responding with local IP...\n");
            // TODO: Return the local IP from database (customize this part as needed)
        }
        else
        {
            printf("Domain not found in intranet. Forwarding the query to Google DNS...\n");
            forward_dns_query_to_google_dns(query_buffer, query_length, socket_fd, client_address);
        }
    }
}

// Main function to set up the DNS server
int main()
{
    int socket_fd;
    struct sockaddr_in server_address;

    // Create UDP socket
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(5354);  // DNS port

    // Bind the socket to the server address and port
    if (bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Bind failed");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    printf("DNS Proxy Server running...\n");

    // Handle incoming DNS queries
    handle_incoming_dns_queries(socket_fd);

    // Close the socket
    close(socket_fd);
    return 0;
}
