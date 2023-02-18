#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>

#include <libvici.h>

typedef struct CommandLineArguments
{
    char* pool_name;
    char* prefix_address;
    int prefix_size;
    int pool_size;
    uint16_t sla_size;
    uint16_t sla_id;
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

/**
 * @brief Converts a command line argument to an integer
 * Prints an error message if the argument is invalid and exits
 * 
 * @param value 
 * @param optionname 
 * @return int 
 */
static int ParseOptionToInteger(char* value, char* optionname);

/**
 * @brief Checks whether the string only contains the character specified
 * Returns true if the string only contains the characters
 * @param string 
 * @param characters 
 * @return bool
 */
static bool CheckOnlyContains(char* string, char* characters);

/**
 * @brief Check whether something is a valid IPv6 address
 * 
 * @param address 
 * @return bool
 */
static bool CheckAddress(char* address);

/**
 * @brief Checks wheter the input arguments are all valid exits if invalid
 * 
 * @param argumenten 
 */
static void CheckValidityOfArguments(CommandLineArguments* argumenten);

/**
 * @brief Formats the output adress to be given to vici
 * 
 * @param argumenten 
 * @param output 
 */
void FormatOutput(CommandLineArguments* arguments, char* output);


int main(int argc, char** argv)
{
    // Parse command line arguments
    CommandLineArguments arguments;
    ParseCommandLineArguments(argc, argv, &arguments);
    CheckValidityOfArguments(&arguments); // Can exit here
    
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
    arguments->prefix_address = NULL;
    arguments->pool_size = 0;
    arguments->prefix_size = 0;
    arguments->sla_id = 0;
    arguments->sla_size = 0;

    while((opt = getopt(argc, argv, ":p:n:s:l:i:j:h")) != -1)
    {
        switch (opt)
        {
        case 'h': // Help
            Usage();
            exit(1);
        case 'n': // Pool name
            arguments->pool_name = optarg;
            break;
        case 'p': // Prefix adress
            arguments->prefix_address = optarg;
            break;
        case 'l': // Prefix size from provider
            arguments->prefix_size = ParseOptionToInteger(optarg, "prefix_size");
            break;
        case 's': // Pool size suffix for vici
            arguments->pool_size = ParseOptionToInteger(optarg, "pool_size");
            break;
        case 'i': // Sla_id
            arguments->sla_id = ParseOptionToInteger(optarg, "Sla_id");
            break;
        case 'j':
            arguments->sla_size = ParseOptionToInteger(optarg, "sla_size");
            break;
        case ':':
            puts("All options require arguments");
            exit(1);
        case '?':
            puts("Unrecognized argument");
            exit(1);
        }
    }

    // Check whether a pool name and a prefix_address is defined
    if(   arguments->pool_name == NULL 
       || arguments->prefix_address == NULL
       || arguments->pool_size == 0)
    {
        // No pool name or prefix_address was defined
        puts("pool_name and prefix_address are required, see -h for usage");
        exit(1);
    }

    // If a provider prefix is provided or a slaid or a slasize, they to be all present
    // Sla_id may be zero
    if(arguments->sla_size != 0  || arguments->prefix_size != 0)
    {
        if(arguments->sla_size == 0 && arguments->prefix_size == 0)
        {
            puts("sla_id, sla_size and prefix_size have to be present together");
            exit(1);
        }
    }
}

static void Usage()
{
    printf("usage: dynamicprefixvici [-h] -n pool_name -p prefix_address [-s pool_size] [-l prefix_size] [-i sla_id] [-j sla_size]\n"
            "-h displays the usage message\n"
            "-n pool_name: sets the pool name to add\n"
            "-p prefix_address: sets the prefix_address to add\n"
            "-s pool_size: size of the pool (/120) as decimal integer\n"
            "-l prefix_size: size of the prefix provided from the provider as decimal integer\n"
            "-i sla_id: sla_id to be appended as decimal integer\n"
            "-j sla_size: size of the sla_id as decimal integer\n");
}

static int ParseOptionToInteger(char* value, char* optionname)
{
    if(isdigit(value[0]))
    {
        return atoi(value);
    }
    else
    {
        printf("%s requires a decimal integer as value\n", optionname);
        exit(1);
    }
}

static bool CheckOnlyContains(char* string, char* characters)
{
    bool result = true;

    int lengt_of_string = strlen(string);
    int lengt_of_characters = strlen(characters);

    for(int i = 0; i < lengt_of_string; i++)
    {
        // Check each character of the string
        bool character_found = false;

        for(int j = 0; j < lengt_of_characters; j++)
        {
            if(string[i] == characters[j])
            {
                // If the character is found it can start with the next character of the string
                character_found = true;
                break; // Goes to the end of the first for loop
            }
        }

        // If the character was not found, character_found remains false and the function must return false
        if(character_found == false)
        {
            result = false;
            break;
        }

        // Else it can start checking the next character of the string
    }

    // If no unspecified character is found, result stays true

    return result;
}

static bool CheckAddress(char* address)
{
    // The address may only contain 0 to 9 and A to F or a to f and :. 
    // We expect it to be full length
    // The length should be 21 characters
    if(    strlen(address) == 21 
        && CheckOnlyContains(address, "0123456789abcdefABCDEF:"))
    {
        return true;
    }
    else 
    {
        return false;
    }
}

static void CheckValidityOfArguments(CommandLineArguments* arguments)
{
    bool validity = false;
    
    // Check prefix_size
    if(arguments->prefix_size >= 64)
    {
        puts("Prefixsize has to be smaller than 64");
    }
    else if(arguments->pool_size < 97)
    {
        puts("Poolsize has to be larger than 96");
    }
    else if(!CheckAddress(arguments->prefix_address))
    {
        // Address is invalid
        puts("Invalid address");
    }
    else if(arguments->sla_size > (64- arguments->prefix_size))
    {
        printf("Slasize: %i \n prefix_size: %i\n", arguments->sla_size, arguments->prefix_size);
        puts("Slasize is larger than prefix allows");
    }
    else if((int)log2(arguments->sla_id) > arguments->sla_size) // log2 gives the amount of binary numbers back apparently; can be done in a different way but it looks interesting
    {
        puts("sla_id is larger tha sla_size allows");
    }
    else
    {
        // Everything was all right
        validity = true;
    }

    // Exit if false
    if(validity == false)
    {
        exit(1);
    }
}

void FormatOutput(CommandLineArguments* arguments, char* output)
{
    //char output[50] = {0}; // More than long enough
    strcpy(output, arguments->prefix_address);

    // Things happen in the last 4 characters before ::
    char* last_characters = &output[15]; // the last 4 characters start at the 15th character; Can be done in a better way but this was the easiest
    
    // Convert the last 4 characters to a 16 bit number
    uint16_t sla = (uint16_t)strtoul(last_characters, NULL, 16);
    // Clear the amount of bits specified by the sla length
    // Create a bitmask
    uint16_t bitmask = 0b1111111111111111; // We could also use 0xff (GCC does not support delimeters ' as specified in c2x)
    bitmask = bitmask << arguments->sla_size; // Shift left 0 is appended

    sla &= bitmask; // Logical AND

    // The suffix clear, now we can append the id
    sla |= arguments->sla_id;

    // Now we can convert it back into a hexadecimal string
    char suffix[15] = {0};
    sprintf(suffix, "%x", sla);
    // Determin the length, maybe a 0 has to be appended in front
    int length = strlen(suffix);
    if(length < 4)
    {
        char workingstring[5] = {0};
        strcpy(&workingstring[4-length], suffix);
        strcpy(suffix, workingstring);
    }

    // Now the pool_size can be appended
    suffix[4] = ':'; 
    suffix[5] = ':';
    suffix[6] = '/';
    sprintf(&suffix[7], "%u", arguments->pool_size);

    // Now the suffix can be placed back in the result
    strcpy(&output[15], suffix);
}
