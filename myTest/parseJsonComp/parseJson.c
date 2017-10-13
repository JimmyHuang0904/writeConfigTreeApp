#include "legato.h"
#include "interfaces.h"
#include <curl/curl.h>

//---------------------------------------------------
/**
 
 */
//---------------------------------------------------

typedef enum
{
    LE_AVDATA_DATA_TYPE_NONE = 0,
    LE_AVDATA_DATA_TYPE_INT = 1,
    LE_AVDATA_DATA_TYPE_FLOAT = 2,
    LE_AVDATA_DATA_TYPE_BOOL = 3,
    LE_AVDATA_DATA_TYPE_STRING = 4
} le_avdata_DataType_t;




le_json_ParsingSessionRef_t jsonSessionRef = NULL;
le_mem_PoolRef_t PointPool;




int Entries = 0;
bool flagType = false;
bool flagPath = false;

// Header declaration
static void SigTermEventHandler(int sigNum);

typedef struct {
    le_avdata_DataType_t dataType;  ///< Type of data
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
        //LE_INFO("dataType = %i", ConfigEntries[i].dataType);
        LE_INFO("configTreePathPtr = %s", ConfigEntries[i].configTreePathPtr);
        LE_INFO("resourcePathPtr = %s", ConfigEntries[i].resourcePathPtr);
    }
}

static void JsonEventHandler
(
    le_json_Event_t event
)
{
    const char * stringBuffer;
    
    // const char * eventname = le_json_GetEventName(event);

    // LE_INFO("event name = %s", eventname);

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
            // New entry

            //PointPool = le_mem_ExpandPool( PointPool , Entries );

            ConfigEntry_t* ConfigEntries = le_mem_ForceAlloc(PointPool);

            //ConfigEntries[Entries].resourcePathPtr = "asdf";

            strcpy(ConfigEntries[Entries].resourcePathPtr, stringBuffer);

            LE_INFO("passed");

            Entries++;
        }
    }

    if (event == LE_JSON_STRING)
    {
        stringBuffer = le_json_GetString();

        if(flagType)
        {
            //LE_INFO(" dataType is : %s", stringBuffer);

            // if( strcmp(stringBuffer, "string") )
            // {
            //     ConfigEntries[Entries-1].dataType = LE_AVDATA_DATA_TYPE_STRING;
            // }
            // else if ( strcmp(stringBuffer, "int") )
            // {
            //     ConfigEntries[Entries-1].dataType = LE_AVDATA_DATA_TYPE_INT;
            // }
            // else if ( strcmp(stringBuffer, "float") )
            // {
            //     ConfigEntries[Entries-1].dataType = LE_AVDATA_DATA_TYPE_FLOAT;
            // }
            // else if ( strcmp(stringBuffer, "bool") )
            // {
            //     ConfigEntries[Entries-1].dataType = LE_AVDATA_DATA_TYPE_BOOL;
            // }
            // else
            // {
            //     LE_ERROR("datatype not define. JSON Parsing has stopped/ Terminating App");
            //     SigTermEventHandler(1);
            // }

            // LE_INFO("in flagtype, %i, entry number = %i ", ConfigEntries[Entries-1].dataType, Entries-1);
            flagType = false;
        }
        else if(flagPath)
        {
            LE_INFO(" datapath is %s entry number is %i", stringBuffer, Entries-1);

            strcpy(ConfigEntries->configTreePathPtr, stringBuffer);

            LE_INFO(" datapath is %s entry number is %i", ConfigEntries->configTreePathPtr, Entries-1);

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
    LE_INFO("In error handler");
    LE_INFO("Msg = %s", msg);

    if ( error == LE_JSON_READ_ERROR)
    {
        LE_INFO("Read error");
    }
    else if ( error == LE_JSON_SYNTAX_ERROR)
    {
        LE_INFO("Syntax error");
    }
    else
        LE_INFO("Weird error");
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
        free(ConfigEntries[i].configTreePathPtr);
    }

}

COMPONENT_INIT
{
    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, SigTermEventHandler);
    int fileDescriptor = open("config.json", O_RDONLY);

    if(fileDescriptor < 0)
    {
        LE_ERROR("fileDescriptor is less than 0");
    }
    else
    {
        LE_INFO("fileDescriptor = %i", fileDescriptor);
    }

    PointPool = le_mem_CreatePool("ConfigEntriesMemPool",sizeof(ConfigEntry_t));
    
    jsonSessionRef = le_json_Parse(fileDescriptor, JsonEventHandler, ErrorHandler, NULL);
}
