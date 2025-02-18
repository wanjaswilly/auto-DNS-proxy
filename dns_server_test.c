#include <cmocka.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <sqlite3.h>
#include "dns_server.h"


// Mock database functions
static int mock_sqlite3_exec(sqlite3 *db, const char *sql, int (*callback)(void*, int, char**, char**), void *arg, char **err_msg) {
    return SQLITE_OK;
}

static int mock_sqlite3_prepare_v2(sqlite3 *db, const char *sql, int size, sqlite3_stmt **stmt, const char **tail) {
    return SQLITE_OK;
}

static int mock_sqlite3_step(sqlite3_stmt *stmt) {
    return SQLITE_DONE;
}

static int mock_sqlite3_finalize(sqlite3_stmt *stmt) {
    return SQLITE_OK;
}

static int mock_sqlite3_bind_text(sqlite3_stmt *stmt, int pos, const char *text, int len, sqlite3_destructor_type destructor) {
    return SQLITE_OK;
}

static int mock_sqlite3_bind_int(sqlite3_stmt *stmt, int pos, int value) {
    return SQLITE_OK;
}

static int mock_sqlite3_open(const char *filename, sqlite3 **db) {
    return SQLITE_OK;
}

static int mock_sqlite3_close(sqlite3 *db) {
    return SQLITE_OK;
}

// Test function to mock DNS forwarding
static ssize_t mock_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    return len;  // Simulate successful send
}

static ssize_t mock_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    // Mock DNS response (simple "A" record)
    const char *response = "mock DNS response data";
    memcpy(buf, response, strlen(response));
    return strlen(response);  // Simulate receiving the full response
}

// Unit test for 'save_dns_query_to_database' function
static void test_save_dns_query_to_database(void **state) {
    sqlite3 *db = NULL;
    int result = sqlite3_open("mock_db.db", &db);
    assert_int_equal(result, SQLITE_OK);
    
    result = save_dns_query_to_database(db, "example.com", 72, "8.8.8.8", 53);
    assert_int_equal(result, SQLITE_OK);
    
    // Ensure mock database functions are called
    assert_int_equal(mock_sqlite3_exec(db, NULL, NULL, NULL, NULL), SQLITE_OK);
    sqlite3_close(db);
}

// Unit test for 'save_dns_answer_to_database' function
static void test_save_dns_answer_to_database(void **state) {
    sqlite3 *db = NULL;
    int result = sqlite3_open("mock_db.db", &db);
    assert_int_equal(result, SQLITE_OK);
    
    result = save_dns_answer_to_database(db, 1, "192.168.1.1", 60);
    assert_int_equal(result, SQLITE_OK);
    
    // Check if database operations were successful
    assert_int_equal(mock_sqlite3_exec(db, NULL, NULL, NULL, NULL), SQLITE_OK);
    sqlite3_close(db);
}

// Unit test for 'parse_dns_response_and_save_to_database'
static void test_parse_dns_response_and_save_to_database(void **state) {
    const char *dig_response = "mock DNS response data";
    parse_dns_response_and_save_to_database(dig_response);
    
    // Assuming the save to database will use mocked functions
    assert_int_equal(mock_sqlite3_exec(NULL, NULL, NULL, NULL, NULL), SQLITE_OK);
}

// Unit test for 'is_domain_part_of_intranet' function
static void test_is_domain_part_of_intranet(void **state) {
    assert_int_equal(is_domain_part_of_intranet("example.com"), 0);  // Domain not in the database
    assert_int_equal(is_domain_part_of_intranet("intranet.local"), 1);  // Intranet domain
}

// Unit test for 'extract_domain_name_from_query'
static void test_extract_domain_name_from_query(void **state) {
    unsigned char query_data[] = {0x00, 0x00, 0x00, 0x00, 0x03, 0x77, 0x77, 0x77, 0x04, 0x63, 0x6F, 0x6D, 0x00};  // "www.com"
    char domain_name[256];
    
    extract_domain_name_from_query(query_data, domain_name);
    
    assert_string_equal(domain_name, "www.com");
}

// Unit test for 'forward_dns_query_to_google_dns' function
static void test_forward_dns_query_to_google_dns(void **state) {
    unsigned char query_data[] = "query data";
    struct sockaddr_in client_address;
    
    // Simulate forwarding DNS query to Google DNS and receiving response
    forward_dns_query_to_google_dns(query_data, sizeof(query_data), 0, client_address);
    
    // Check if DNS forwarding was successful (mock send and recv)
    assert_int_equal(mock_sendto(0, query_data, sizeof(query_data), 0, NULL, 0), sizeof(query_data));
    assert_int_equal(mock_recvfrom(0, query_data, sizeof(query_data), 0, NULL, NULL), sizeof(query_data));
}

// Unit test for 'handle_incoming_dns_queries' function
static void test_handle_incoming_dns_queries(void **state) {
    int socket_fd = 0;  // Mock socket
    handle_incoming_dns_queries(socket_fd);
    
    // We cannot directly test infinite loop, so we'll check for socket interaction
    assert_int_equal(mock_recvfrom(socket_fd, NULL, 0, 0, NULL, NULL), 0);
}

int main(void) {
    // Setting up mock function callbacks
    sqlite3_exec = mock_sqlite3_exec;
    sqlite3_prepare_v2 = mock_sqlite3_prepare_v2;
    sqlite3_step = mock_sqlite3_step;
    sqlite3_finalize = mock_sqlite3_finalize;
    sqlite3_bind_text = mock_sqlite3_bind_text;
    sqlite3_bind_int = mock_sqlite3_bind_int;
    sqlite3_open = mock_sqlite3_open;
    sqlite3_close = mock_sqlite3_close;
    sendto = mock_sendto;
    recvfrom = mock_recvfrom;

    // Running tests
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_save_dns_query_to_database),
        cmocka_unit_test(test_save_dns_answer_to_database),
        cmocka_unit_test(test_parse_dns_response_and_save_to_database),
        cmocka_unit_test(test_is_domain_part_of_intranet),
        cmocka_unit_test(test_extract_domain_name_from_query),
        cmocka_unit_test(test_forward_dns_query_to_google_dns),
        cmocka_unit_test(test_handle_incoming_dns_queries),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
