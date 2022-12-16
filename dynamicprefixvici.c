#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libvici.h>
#include <unistd.h>

typedef struct CommandLineArguments
{
    char* poolName;
    char* oldPrefixEnv;
    char* newPrefixEnv;
} CommandLineArguments;


/**
  * @brief Retrieves a given environment value by name and places it in an allocated place in memory
  *
  * @param value The name of the environment value
  * @return char* An allocated string in memory with the found string, NULL in case the environment value does not exist
*/
static char* getEnvironmentValue(const char* value);

/**
  * @brief Creates a load-pool message
  *
  * @param poolName name the pool should have
  * @param addressPool The address pool
  * @return vici_req_t* The message to send
*/
static vici_req_t* createMessage(char* poolName, char* addressPool);

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
    char* newPrefixEnv = "new_dhcp6_ia_pd1_prefix1";
    char* oldPrefixEnv = "old_dhcp6_ia_pd1_prefix1";
    char* poolName;
    
    if(arguments.newPrefixEnv != NULL)
    {
        newPrefixEnv = arguments.newPrefixEnv;
    }
    if(arguments.oldPrefixEnv != NULL)
    {
        oldPrefixEnv = arguments.oldPrefixEnv;
    }
    if(arguments.poolName != NULL)
    {
        poolName = arguments.poolName;
    }

    // Get environment values
    char* newPrefix = getEnvironmentValue(newPrefixEnv);
    char* oldPrefix = getEnvironmentValue(oldPrefixEnv);
    
    // 20kB of space in memory to compose a message
    // A place in memory where the diagnostic message can be stored
    char betweenMessage[4096] = {0};
    char diagnosticMessage[16384] = {0};

    // Check if a new prefix is defined
    if(newPrefix != NULL)
    {
        // Check whether only a new prefix is defined or the new prefix is different from the old prefix
        if(oldPrefix == NULL || strcmp(newPrefix, oldPrefix))
        {
            // They are different, add to dianogstic message
            if(oldPrefix != NULL)
            {
                sprintf(betweenMessage, "oldPrefix: %s\n", oldPrefix);
                strcat(diagnosticMessage, betweenMessage);
            }
            sprintf(betweenMessage, "newPrefix: %s\n", newPrefix);
            strcat(diagnosticMessage, betweenMessage);

            // Initialize vici library
            vici_init();

            // Create addressPool, length old string + "/120" + 0 at the end
            char* addressPool = malloc(sizeof(char) * (strlen(newPrefix) + 4 + 1));
            strcpy(addressPool, newPrefix);
            strcat(addressPool, "/120");

            sprintf(betweenMessage, "addressPool: %s\n", addressPool);
            strcat(diagnosticMessage, betweenMessage);

            vici_req_t* messageToSend = createMessage(poolName, addressPool);
            
            // Open connection, assume the system is the standard connection
            vici_conn_t* connection = vici_connect(NULL); 

            if(connection != NULL)
            {
                // Connection has been made, forward message
                vici_res_t* response = vici_submit(messageToSend, connection);

                // Check if the message has been sent correctly
                if(response == NULL)
                {
                    sprintf(betweenMessage, "Status: Unable to send the message: %s\n", strerror(errno));
                    strcat(diagnosticMessage, betweenMessage);
                }
                else
                {
                    /*// Check if the addressPool is actually added
                    vici_parse_t contect = vici_parse(response); // should return VICI_PARSE_BEGIN_SECTION
                    contect = vici_parse(response); // should return VICI_PARSE_KEY_VALUE
                    if(contect == VICI_PARSE_KEY_VALUE)
                    {
                        printf("%s\n", vici_parse_value_str(response));
                    }
                    else
                    {
                        // message looks different than expected
                        sprintf(betweenMessage, "Different response than expected");
                        strcat(diagnosticMessage, betweenMessage);
                    }
                    */

                    // Add to dianogstic message
                    sprintf(betweenMessage, "Received message: \n%s\n", (char*)response); 
                    strcat(diagnosticMessage, betweenMessage);
                }

                vici_disconnect(connection);
            }
            else
            {
                // Add to dianogstic message
                sprintf(betweenMessage, "Status: Connection failed: %s\n", strerror(errno));
                strcat(diagnosticMessage, betweenMessage);
            }
        }
        else
        {
            strcat(diagnosticMessage, "Status: Addresses are the same\n");
        }
    }
    else
    {
        // No new prefix is defined
        strcat(diagnosticMessage, "Status: No environment variable for new prefix");
    }

    puts(diagnosticMessage);
    return 0;
}

static char* getEnvironmentValue(const char* value)
{
    char* readValue = NULL;
    // It cannot be guaranteed that the content of the pointer will remain the same on a subsequent getenv call, so make a copy
    char* environmentValue = getenv(value); 

    if(environmentValue != NULL)
    {
        size_t lengte = strlen(environmentValue);
        readValue = malloc(lengte + 1);
        strcpy(readValue, environmentValue);
    }

    return readValue;
}

static vici_req_t* createMessage(char* poolName, char* addressPool)
{
    /*
    <pool name> = {
        addrs = <subnet of vitual IP pool adreses>
        <Attrite type>* = ...
    }
    */

    vici_req_t* message = vici_begin("load-pool");
    vici_begin_section(message, poolName);
    vici_add_key_value(message, "addrs", addressPool, (int)strlen(addressPool));
    vici_end_section(message);

    return message;
}

static void ParseCommandLineArguments(int argc, char** argv, CommandLineArguments* arguments)
{
    int opt;
    opterr = 0; // no error message by getopt
    arguments->poolName = NULL;
    arguments->newPrefixEnv = NULL;
    arguments->oldPrefixEnv = NULL;

    while((opt = getopt(argc, argv, ":p:n:o:h")) != -1)
    {
        switch (opt)
        {
        case 'h':// Help
            Usage();
            exit(1);
            break;
        case 'p': // Pool name
            arguments->poolName = optarg;
            break;
        case 'n':
            arguments->newPrefixEnv = optarg;
            break;
        case 'o':
            arguments->oldPrefixEnv = optarg;
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
    if(arguments->poolName == NULL)
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