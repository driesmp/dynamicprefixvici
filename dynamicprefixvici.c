#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libvici.h>

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

int main(void)
{
    char* newPrefix = getEnvironmentValue("new_dhcp6_ia_pd1_prefix1");
    char* oldPrefix = getEnvironmentValue("old_dhcp6_ia_pd1_prefix1");
    char* poolName = "global-ipv6";

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
