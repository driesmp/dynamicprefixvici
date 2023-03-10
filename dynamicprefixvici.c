#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libvici.h>

typedef struct CommandLineArguments
{
    char* pool_name;
    char* prefix_address;
    char* pool_size;
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
 * @brief Formats the output address to be given to vici
 * 
 * @param arguments 
 * @param output 
 */
static void FormatOutput(CommandLineArguments* arguments, char* address_pool);

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

    // Add new prefix_address to diagnostic message
    sprintf(between_message, "prefix_address: %s\n", arguments.prefix_address);
    strcat(diagnostic_message, between_message);

    // Initialize vici library
    vici_init();

    // Create address_pool to be sent
    char address_pool[50] = {0};
    FormatOutput(&arguments, address_pool);
    
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
            char* parsed_message;
            if(vici_parse(response) == VICI_PARSE_KEY_VALUE)
            {
                parsed_message = vici_parse_value_str(response);
            }
            else
            {
                // Unable to parse other than file
                parsed_message = "ERROR: Unable to parse";
            }

            sprintf(between_message, "Received message: %s\n", parsed_message);
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

static void FormatOutput(CommandLineArguments* arguments, char* address_pool)
{
    // Append the pool size to the address
    strcpy(address_pool, arguments->prefix_address);

    size_t length_string = strlen(output);
    sprintf(&address_pool[length_string], "/%s", arguments->pool_size);
}

static void ParseCommandLineArguments(int argc, char** argv, CommandLineArguments* arguments)
{
    int opt;
    opterr = 0; // no error message by getopt
    arguments->pool_name = NULL;
    arguments->prefix_address = NULL;
    arguments->pool_size = "97"; // default chosen as the largest size strongSwan will add

    while((opt = getopt(argc, argv, ":p:n:s:h")) != -1)
    {
        switch (opt)
        {
        case 'h':
            Usage();
            exit(1);
        case 'n':
            arguments->pool_name = optarg;
            break;
        case 'p':
            arguments->prefix_address = optarg;
            break;
        case 's':
            arguments->pool_size = optarg;
            break;
        case ':':
            puts("All options require arguments");
            exit(1);
        case '?':
            puts("Unrecognized argument");
            exit(1);
        }
    }

    // Check whether a pool_name and a prefix_address are defined
    if(arguments->pool_name == NULL || arguments->prefix_address == NULL)
    {
        puts("pool_name and prefix_address are required, see -h for usage");
        exit(1);
    }
}

static void Usage()
{
    printf("usage: dynamicprefixvici [-h] -n pool_name -p prefix_address [-s pool_size[97]]\n"
            "-h displays the usage message\n"
            "-n pool_name: sets the pool name to add\n"
            "-p prefix_address: sets the prefix_address to add\n"
            "-s pool_size: size of the pool to add as decimal integer\n");
}
