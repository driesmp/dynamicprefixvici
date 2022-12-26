#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libvici.h>
#include <unistd.h>

typedef struct CommandLineArguments
{
    char* pool_name;
    char* old_prefix_env;
    char* new_prefix_env;
} CommandLineArguments;


/**
  * @brief Retrieves a given environment value by name and places it in an allocated place in memory
  *
  * @param value The name of the environment value
  * @return char* An allocated string in memory with the found string, NULL in case the environment value does not exist
*/
static char* GetEnvironmentValue(const char* value);

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

    // Set default values
    char* new_prefix_env = "new_dhcp6_ia_pd1_prefix1";
    char* old_prefix_env = "old_dhcp6_ia_pd1_prefix1";
    char* pool_name;
    
    if(arguments.new_prefix_env != NULL)
    {
        new_prefix_env = arguments.new_prefix_env;
    }
    if(arguments.old_prefix_env != NULL)
    {
        old_prefix_env = arguments.old_prefix_env;
    }
    if(arguments.pool_name != NULL)
    {
        pool_name = arguments.pool_name;
    }

    // Get environment values
    char* new_prefix = GetEnvironmentValue(new_prefix_env);
    char* old_prefix = GetEnvironmentValue(old_prefix_env);
    
    // 20kB of space in memory to compose a message
    // A place in memory where the diagnostic message can be stored
    char between_message[4096] = {0};
    char diagnostic_message[16384] = {0};

    // Check if a new prefix is defined
    if(new_prefix != NULL)
    {
        // Check whether only a new prefix is defined or the new prefix is different from the old prefix
        if(old_prefix == NULL || strcmp(new_prefix, old_prefix))
        {
            // They are different, add to dianogstic message
            if(old_prefix != NULL)
            {
                sprintf(between_message, "old_prefix: %s\n", old_prefix);
                strcat(diagnostic_message, between_message);
            }
            sprintf(between_message, "new_prefix: %s\n", new_prefix);
            strcat(diagnostic_message, between_message);

            // Initialize vici library
            vici_init();

            // Create address_pool, length old string + "/120" + 0 at the end
            char* address_pool = malloc(sizeof(char) * (strlen(new_prefix) + 4 + 1));
            strcpy(address_pool, new_prefix);
            strcat(address_pool, "/120");

            sprintf(between_message, "address_pool: %s\n", address_pool);
            strcat(diagnostic_message, between_message);

            vici_req_t* message_to_send = CreateMessage(pool_name, address_pool);
            
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
        }
        else
        {
            strcat(diagnostic_message, "Status: Addresses are the same\n");
        }
    }
    else
    {
        // No new prefix is defined
        strcat(diagnostic_message, "Status: No environment variable for new prefix");
    }

    puts(diagnostic_message);
    return 0;
}

static char* GetEnvironmentValue(const char* value)
{
    char* read_value = NULL;
    // It cannot be guaranteed that the content of the pointer will remain the same on a subsequent getenv call, so make a copy
    char* environment_value = getenv(value); 

    if(environment_value != NULL)
    {
        size_t lengte = strlen(environment_value);
        read_value = malloc(lengte + 1);
        strcpy(read_value, environment_value);
    }

    return read_value;
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
    arguments->new_prefix_env = NULL;
    arguments->old_prefix_env = NULL;

    while((opt = getopt(argc, argv, ":p:n:o:h")) != -1)
    {
        switch (opt)
        {
        case 'h':// Help
            Usage();
            exit(1);
            break;
        case 'p': // Pool name
            arguments->pool_name = optarg;
            break;
        case 'n':
            arguments->new_prefix_env = optarg;
            break;
        case 'o':
            arguments->old_prefix_env = optarg;
            break;
        case ':':
            puts("All options require arguments");
            exit(1);
            break;
        case '?':
            puts("Unrecognized argument");
            exit(1);
            break;
        }
    }

    // Check whether a pool name is defined
    if(arguments->pool_name == NULL)
    {
        // No pool name was defined

        puts("Pool name is required, see -h for usage");
        exit(1);
    }
}

static void Usage()
{
    printf("usage: dynamicprefixvici [-h] [-n newPrefixName] [-o oldPrefixName] -p poolName\n"
            "-h help message\n"
            "-n newPrefixName: set name of environment variable which contains new prefix\n"
            "-o oldPrefixName: set name of environment variable which contains old prefix\n"
            "-p poolName: pool name added to strongSwan\n"
            );
}