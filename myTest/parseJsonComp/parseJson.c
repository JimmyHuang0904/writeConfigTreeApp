#include "legato.h"
#include "interfaces.h"
#include <curl/curl.h>

//---------------------------------------------------
/**
 
 */
//---------------------------------------------------

le_json_ParsingSessionRef_t jsonSessionRef = NULL;

int Entries = 0;
bool flagType = false;
bool flagPath = false;

typedef struct {
    char* dataType;  ///< Type of data
    char * configTreePathPtr; ///< Path to data in the configTree.
    char * resourcePathPtr;   ///< Path to the resource when accessed from AVC.
} ConfigEntry_t;

ConfigEntry_t * ConfigEntries;

static void test()
{
    int i = 0;
    LE_INFO(" size of config entry = %i", Entries);
    
    for(i = 0; i < Entries; i++)
    {
        LE_INFO("dataType = %s", ConfigEntries[i].dataType);
        LE_INFO("configTreePathPtr = %s", ConfigEntries[i].configTreePathPtr);
    }
}

static void JsonEventHandler
(
    le_json_Event_t event
)
{
    const char * stringBuffer;

    const char * eventname = le_json_GetEventName(event);

    LE_INFO("event name = %s", eventname);
    
    if (event == LE_JSON_OBJECT_MEMBER)
    {
        stringBuffer = le_json_GetString();
        LE_INFO("%s", stringBuffer);
        if( !strcmp(stringBuffer, "type") )
        {
            flagType = true;
        }
        else if( !strcmp(stringBuffer, "path") )
        {
            flagPath = true;
        }
        else
        {
            Entries++;
            ConfigEntries = realloc(ConfigEntries, Entries*sizeof(ConfigEntry_t));
            ConfigEntries[Entries-1].resourcePathPtr = malloc(sizeof(ConfigEntries[Entries-1].resourcePathPtr));
            strcpy(ConfigEntries[Entries-1].resourcePathPtr, stringBuffer);
        }
    }

    if (event == LE_JSON_STRING)
    {
        stringBuffer = le_json_GetString();

        if(flagType)
        {
            LE_INFO(" dataType is : %s, entry number is: %i , size %i ", stringBuffer, Entries-1, (int) strlen(stringBuffer));
        
            ConfigEntries[Entries-1].dataType = malloc(sizeof(ConfigEntries[Entries-1].dataType));
            strcpy(ConfigEntries[Entries-1].dataType, stringBuffer);

            LE_INFO("in flagtype, %s, entry number = %i ", ConfigEntries[Entries-1].dataType, Entries-1);
            flagType = false;
        }
        else if(flagPath)
        {
            LE_INFO(" datapath is %s entry number is %i", stringBuffer, Entries-1);

            ConfigEntries[Entries-1].configTreePathPtr = malloc(sizeof(ConfigEntries[Entries-1].configTreePathPtr));
            strcpy(ConfigEntries[Entries-1].configTreePathPtr, stringBuffer);
            LE_INFO("in flagPath, %s, entry number = %i ", ConfigEntries[Entries-1].configTreePathPtr, Entries-1);
            flagPath = false;
        }
        else
            LE_ERROR("not suppose to end up here");
    }

    if (event == LE_JSON_DOC_END)
    {
        test();
    }
}

static void ErrorHandler
(
    le_json_Error_t error,
    const char * msg
)
{
    LE_INFO("in error handler");
    LE_INFO("msg = %s", msg);

    if ( error == LE_JSON_READ_ERROR)
    {
        LE_INFO("Read error");
    }
    else if ( error == LE_JSON_SYNTAX_ERROR)
    {
        LE_INFO("Syntax error");
    }
    else
        LE_INFO("weird error");
}

static void SigTermEventHandler
(
    int sigNum
)
{
    int i = 0;
    le_json_Cleanup(jsonSessionRef);
    LE_INFO("SigTerm Event handler kill app");

    for(i = 0; i < Entries; i++)
    {
        LE_INFO("free mem");
        free(ConfigEntries[i].resourcePathPtr);
        free(ConfigEntries[i].dataType);
        free(ConfigEntries[i].configTreePathPtr);
    }
    
}

COMPONENT_INIT
{
    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, SigTermEventHandler);
    int fd = open("config.json", O_RDONLY);

    if(fd < 0)
    {
        LE_ERROR("FD is less than 0");
    }

    LE_INFO("hello, fd = %i", fd);

    //ConfigEntries = malloc(Entries*sizeof(ConfigEntry_t));
    jsonSessionRef = le_json_Parse(fd, JsonEventHandler, ErrorHandler, NULL);
}