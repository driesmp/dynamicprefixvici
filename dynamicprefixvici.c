#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libvici.h>

typedef struct CommandLineArguments
{
    char* pool_name;
    char* pool_size;
    char* prefix_address;
} CommandLineArguments;

/**
 * @brief Creates a load-pool message
 *
 * @param pool_name name the pool should have
 * @param address_pool The address pool
 * @return vici_req_t* The message to send
 */
static vici_req_t* CreateLoadPoolMessage(char* pool_name, char* address_pool);

/**
 * @brief Formats the address_pool to be given to vici
 * 
 * @param arguments 
 * @param address_pool 
 */
static void FormatAddressPool(CommandLineArguments* arguments, char* address_pool);

/**
 * @brief Get all command line arguments and gives it back in a struct
 * If unsupported arguments are given, it terminates the program
 *
 * @param argc
 * @param argv
 * @param arguments
 */
static void ParseCommandLineArguments(int argc, char** argv, CommandLineArguments* arguments);

/**
 * @brief Prints the usage of the program
 */
static void Usage(void);


int main(int argc, char** argv)
{
    // parse command line arguments
    CommandLineArguments arguments;
    ParseCommandLineArguments(argc, argv, &arguments);

    // create address_pool to be sent
    char address_pool[50] = {0};
    FormatAddressPool(&arguments, address_pool);
    printf("address_pool: %s\n", address_pool);

    // initialize vici library
    vici_init();
    
    // create load-pool message
    vici_req_t* load_pool_message = CreateLoadPoolMessage(arguments.pool_name, address_pool);

    // open connection, assume the system is the standard connection
    vici_conn_t* connection = vici_connect(NULL);

    if(connection != NULL)
    {
        // connection has been made, forward message
        vici_res_t* response = vici_submit(load_pool_message, connection);

        // check if the message has been sent correctly
        if(response == NULL)
        {
            printf("Unable to send the message: %s\n", strerror(errno));
        }
        else
        {
            char* parsed_message;
            if(vici_parse(response) == VICI_PARSE_KEY_VALUE)
            {
                parsed_message = vici_parse_value_str(response);
                // let the program exit with 1 if vici was unable to add the pool
                if(parsed_message[0] != 'y')
                {
                    // parse the error message from vici
                    parsed_message = vici_parse_value_str(response);
                    puts(parsed_message);
                    exit(1);
                }
            }
            else
            {
                parsed_message = "ERROR: Unable to parse return message";
            }

            printf("Received message: %s\n", parsed_message);
        }

        vici_disconnect(connection);
    }
    else
    {
        printf("Connection failed: %s\n", strerror(errno));
    }

    return 0;
}

static vici_req_t* CreateLoadPoolMessage(char* pool_name, char* address_pool)
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

static void FormatAddressPool(CommandLineArguments* arguments, char* address_pool)
{
    // copy the prefix_address to the address_pool
    strcpy(address_pool, arguments->prefix_address);
    size_t length_string = strlen(address_pool);
    // append a "/" with the pool_size
    sprintf(&address_pool[length_string], "/%s", arguments->pool_size);
}

static void ParseCommandLineArguments(int argc, char** argv, CommandLineArguments* arguments)
{
    int opt;
    opterr = 0; // no error message by getopt
    arguments->pool_name = NULL;
    arguments->pool_size = "97"; // default chosen as the largest size strongSwan will add
    arguments->prefix_address = NULL;

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

    // check whether a pool_name and a prefix_address are defined
    if(arguments->pool_name == NULL || arguments->prefix_address == NULL)
    {
        puts("pool_name and prefix_address are required, see -h for usage");
        exit(1);
    }
}

static void Usage(void)
{
    puts("usage: dynamicprefixvici [-h] -n pool_name -p prefix_address [-s pool_size[97]]\n"
            "-h displays the usage message\n"
            "-n pool_name: sets the pool name to add\n"
            "-p prefix_address: sets the prefix_address to add\n"
            "-s pool_size: size of the pool to add as decimal integer");
}
