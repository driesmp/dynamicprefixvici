#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libvici.h>
#include <unistd.h>

typedef struct CommandLineArguments
{
    char* pool_name;
    char* prefix;
} CommandLineArguments;

/**
 * @brief Creates a load-pool message
 *
 * @param pool_name name the pool should have
 * @param address_pool The address pool
 * @return vici_req_t* The message to send
 */
static vici_req_t* CreateMessage(char* pool_name, char* address_pool);

/**
 * @brief get all command line arguments and gives it back in a struct.
 * If unsupported arguments are given, it terminates the program
 *
 * @param argc
 * @param argv
 * @param arguments
 */
static void ParseCommandLineArguments(int argc, char** argv, CommandLineArguments* arguments);

/**
 * @brief prints the usage of the program
 */
static void Usage();

int main(int argc, char** argv)
{
    // Parse command line arguments
    CommandLineArguments arguments;
    ParseCommandLineArguments(argc, argv, &arguments);

    // 20kB of space in memory to compose a message
    // A place in memory where the diagnostic message can be stored
    char between_message[4096] = {0};
    char diagnostic_message[16384] = {0};

    // Add new prefix to diagnostic message
    sprintf(between_message, "prefix: %s\n", arguments.prefix);
    strcat(diagnostic_message, between_message);

    // Initialize vici library
    vici_init();

    // Create address_pool, length old string + "/120" + 0 at the end
    char* address_pool = malloc(sizeof(char) * (strlen(arguments.prefix) + 4 + 1));
    strcpy(address_pool, arguments.prefix);
    strcat(address_pool, "/120");

    sprintf(between_message, "address_pool: %s\n", address_pool);
    strcat(diagnostic_message, between_message);

    vici_req_t* message_to_send = CreateMessage(arguments.pool_name, address_pool);

    // Open connection, assume the system is the standard connection
    vici_conn_t* connection = vici_connect(NULL);

    if(connection != NULL)
    {
        // Connection has been made, forward message
        vici_res_t* response = vici_submit(message_to_send, connection);

        // Check if the message has been sent correctly
        if(response == NULL)
        {
            sprintf(between_message, "Status: Unable to send the message: %s\n", strerror(errno));
            strcat(diagnostic_message, between_message);
        }
        else
        {
            /*// Check if the address_pool is actually added
            vici_parse_t contect = vici_parse(response); // should return VICI_PARSE_BEGIN_SECTION
            contect = vici_parse(response); // should return VICI_PARSE_KEY_VALUE
            if(contect == VICI_PARSE_KEY_VALUE)
            {
                printf("%s\n", vici_parse_value_str(response));
            }
            else
            {
                // message looks different than expected
                sprintf(between_message, "Different response than expected");
                strcat(diagnostic_message, between_message);
            }
            */

            // Add to dianogstic message
            sprintf(between_message, "Received message: \n%s\n", (char*)response);
            strcat(diagnostic_message, between_message);
        }

        vici_disconnect(connection);
    }
    else
    {
        // Add to dianogstic message
        sprintf(between_message, "Status: Connection failed: %s\n", strerror(errno));
        strcat(diagnostic_message, between_message);
    }

    puts(diagnostic_message);
    return 0;
}

static vici_req_t* CreateMessage(char* pool_name, char* address_pool)
{
    /*
    <pool name> = {
        addrs = <subnet of vitual IP pool adreses>
        <Attrite type>* = ...
    }
    */
    vici_req_t* message = vici_begin("load-pool");
    vici_begin_section(message, pool_name);
    vici_add_key_value(message, "addrs", address_pool, (int)strlen(address_pool));
    vici_end_section(message);

    return message;
}

static void ParseCommandLineArguments(int argc, char** argv, CommandLineArguments* arguments)
{
    int opt;
    opterr = 0; // no error message by getopt
    arguments->pool_name = NULL;
    arguments->prefix = NULL;

    while((opt = getopt(argc, argv, ":p:n:h")) != -1)
    {
        switch (opt)
        {
        case 'h': // Help
            Usage();
            exit(1);
        case 'n': // Pool name
            arguments->pool_name = optarg;
            break;
        case 'p':
            arguments->prefix = optarg;
            break;
        case ':':
            puts("All options require arguments");
            exit(1);
        case '?':
            puts("Unrecognized argument");
            exit(1);
        }
    }

    // Check whether a pool name and a prefix is defined
    if(arguments->pool_name == NULL && arguments->prefix == NULL)
    {
        // No pool name or prefix was defined
        puts("Pool name and prefix are required, see -h for usage");
        exit(1);
    }
}

static void Usage()
{
    printf("usage: dynamicprefixvici [-h] -n poolName -p prefix \n"
            "-h displays the usage message\n"
            "-n poolName: sets the pool name to add\n"
            "-p prefix: sets the prefix to add\n");
}
