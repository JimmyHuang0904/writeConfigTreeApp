#include "legato.h"
#include "interfaces.h"

#define ARRAY_SIZE 512

/* settings*/

// Name of the Config Tree you want to write to
#define CONFIG_TREE_NAME "trafficLight:"

// Paths in the config tree you want to write to
#define CONFIG_TREE_URL "/url"
#define CONFIG_TREE_INFO_EXITCODE_CHECKFLAG "/info/exitCode/checkFlag"
#define CONFIG_TREE_INFO_CONTENT_CHECKFLAG "/info/content/checkFlag"
#define CONFIG_TREE_POLLINGINTERVALSEC "/pollingIntervalSec"

//-------------------------------------------------------------------------------------------------
/**
 * AVC related variable
 */
//-------------------------------------------------------------------------------------------------
// Reference to AVC event handler
le_avdata_SessionStateHandlerRef_t  avcEventHandlerRef = NULL;
// Reference to AVC Session handler
le_avdata_RequestSessionObjRef_t sessionRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Struct defines each config entry's data type, path to config tree node and resource for comparison
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    le_avdata_DataType_t dataType;  ///< Type of data
    const char * configTreePathPtr; ///< Path to data in the configTree.
    void * resourcePathPtr;   ///< Path to the resource when accessed from AVC.
} ConfigEntry_t;

//--------------------------------------------------------------------------------------------------
/**
 * Holds information of the data type, path to where the config tree will be written, and contextPtr
 * tag.
 */
//--------------------------------------------------------------------------------------------------
const ConfigEntry_t ConfigEntries[] = {
    { LE_AVDATA_DATA_TYPE_STRING,
      CONFIG_TREE_URL,
      CONFIG_TREE_URL},
    { LE_AVDATA_DATA_TYPE_INT,
      CONFIG_TREE_POLLINGINTERVALSEC,
      CONFIG_TREE_POLLINGINTERVALSEC},
    { LE_AVDATA_DATA_TYPE_BOOL,
      CONFIG_TREE_INFO_EXITCODE_CHECKFLAG,
      CONFIG_TREE_INFO_EXITCODE_CHECKFLAG},
    { LE_AVDATA_DATA_TYPE_BOOL,
      CONFIG_TREE_INFO_CONTENT_CHECKFLAG,
      CONFIG_TREE_INFO_CONTENT_CHECKFLAG},
    /* ... insert other entries here ... */
};

//-------------------------------------------------------------------------------------------------
/**
 * Functions to write different data types (bool, string, int, float) to the config tree given the
 * path and the data
 */
//-------------------------------------------------------------------------------------------------
static void WriteBoolToConfig
(
    bool dataToWrite,
    const char * pathToDataPtr
)
{
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateWriteTxn(CONFIG_TREE_NAME);
    le_cfg_SetBool(iteratorRef, pathToDataPtr, dataToWrite);
    le_cfg_CommitTxn(iteratorRef);
}

static void WriteStringToConfig
(
    char * dataToWrite,
    const char * pathToDataPtr
)
{
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateWriteTxn(CONFIG_TREE_NAME);
    le_cfg_SetString(iteratorRef, pathToDataPtr, dataToWrite);
    le_cfg_CommitTxn(iteratorRef);
}

static void WriteIntToConfig
(
    int dataToWrite,
    const char * pathToDataPtr
)
{
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateWriteTxn(CONFIG_TREE_NAME);
    le_cfg_SetInt(iteratorRef, pathToDataPtr, dataToWrite);
    le_cfg_CommitTxn(iteratorRef);
}

static void WriteFloatToConfig
(
    double dataToWrite,
    const char * pathToDataPtr
)
{
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateWriteTxn(CONFIG_TREE_NAME);
    le_cfg_SetFloat(iteratorRef, pathToDataPtr, dataToWrite);
    le_cfg_CommitTxn(iteratorRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * This function is returned whenever a write operation occurs from AirVantage.
 * It will write the data sent from the resource to the path specified.
 */
//-------------------------------------------------------------------------------------------------
static void ConfigSettingHandler
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    const char * pathToDataPtr = NULL;
    le_avdata_DataType_t dataType = LE_AVDATA_DATA_TYPE_NONE;

    // Data to be stored with their respective data types
    char bufferString[ARRAY_SIZE] = "";
    int bufferInt;
    bool bufferBool;
    double bufferFloat;
    int i = 0;

    // Find the pathToDataPtr and the dataType through the contextPtr sent by the handler.
    for ( i = 0; i < NUM_ARRAY_MEMBERS(ConfigEntries); i++)
    {
        if ( contextPtr == ConfigEntries[i].resourcePathPtr )
        {
            pathToDataPtr = ConfigEntries[i].configTreePathPtr;
            dataType = ConfigEntries[i].dataType;
        }
    }

    LE_INFO("------------- Server writes to: %s%s -------------", CONFIG_TREE_NAME, (char *) contextPtr);
    LE_INFO("path to data is : %s", pathToDataPtr);

    // Get the data with their respective data types
    if ( dataType == LE_AVDATA_DATA_TYPE_STRING )
    {
        le_result_t resultGetString = le_avdata_GetString(pathToDataPtr, bufferString, ARRAY_SIZE);
        if (LE_FAULT == resultGetString)
        {
            LE_ERROR("Error in getting latest %s", pathToDataPtr);
        }
        le_result_t resultSetString = le_avdata_SetString(pathToDataPtr, bufferString);
        if (LE_FAULT == resultSetString)
        {
            LE_ERROR("Error in getting latest %s", pathToDataPtr);
        }

        WriteStringToConfig(bufferString, pathToDataPtr);
    }
    else if ( dataType == LE_AVDATA_DATA_TYPE_INT )
    {
        le_result_t resultGetInt = le_avdata_GetInt(pathToDataPtr, &bufferInt);
        if (LE_FAULT == resultGetInt)
        {
            LE_ERROR("Error in getting latest %s", pathToDataPtr);
        }
        le_result_t resultSetInt = le_avdata_SetInt(pathToDataPtr, bufferInt);
        if (LE_FAULT == resultSetInt)
        {
            LE_ERROR("Error in getting latest %s", pathToDataPtr);
        }

        WriteIntToConfig(bufferInt, pathToDataPtr);
    }
    else if ( dataType == LE_AVDATA_DATA_TYPE_FLOAT )
    {
        le_result_t resultGetFloat = le_avdata_GetFloat(pathToDataPtr, &bufferFloat);
        if (LE_FAULT == resultGetFloat)
        {
            LE_ERROR("Error in getting latest %s", pathToDataPtr);
        }
        le_result_t resultSetFloat = le_avdata_SetFloat(pathToDataPtr, bufferFloat);
        if (LE_FAULT == resultSetFloat)
        {
            LE_ERROR("Error in getting latest %s", pathToDataPtr);
        }

        WriteFloatToConfig(bufferFloat, pathToDataPtr);
    }
    else if ( dataType == LE_AVDATA_DATA_TYPE_BOOL )
    {
        le_result_t resultGetBool = le_avdata_GetBool(pathToDataPtr, &bufferBool);
        if (LE_FAULT == resultGetBool)
        {
            LE_ERROR("Error in getting latest %s", pathToDataPtr);
        }
        le_result_t resultSetBool = le_avdata_SetBool(pathToDataPtr, bufferBool);
        if (LE_FAULT == resultSetBool)
        {
            LE_ERROR("Error in getting latest %s", pathToDataPtr);
        }

        WriteBoolToConfig(bufferBool, pathToDataPtr);
    }
    else if ( dataType == LE_AVDATA_DATA_TYPE_NONE )
    {
        LE_ERROR("Error: dataType was not assigned (type none), pathToDataPtr may be corrupt");
    }
    else
    {
        LE_ERROR("Error, not a valid dataType or wrong contextPtr");
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Function relevant to AirVantage server connection
 */
//-------------------------------------------------------------------------------------------------
static void AppTerminationHandler
(
    int sigNum
)
{
    LE_INFO("Close AVC session");
    le_avdata_ReleaseSession(sessionRef);
    if (NULL != avcEventHandlerRef)
    {
        // Unregister
        LE_INFO("Unregister the session handler");
        le_avdata_RemoveSessionStateHandler(avcEventHandlerRef);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Status handler for avcService updates
 */
//-------------------------------------------------------------------------------------------------
static void AvcStatusHandler
(
    le_avdata_SessionState_t updateStatus,
    void* contextPtr
)
{
    switch (updateStatus)
    {
        case LE_AVDATA_SESSION_STARTED:
            LE_INFO("Legato session started successfully");
            break;
        case LE_AVDATA_SESSION_STOPPED:
            LE_INFO("Legato session stopped");
            break;
        default:
            LE_ERROR("Error: AvcStatusHandler called with no updateStatus passed to the function");
    }
}

COMPONENT_INIT
{
    le_result_t resultCreateResources;
    int i = 0;

    LE_INFO("Start Legato writeConfigTree App");

    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, AppTerminationHandler);

    // Start AVC Session
    // Register AVC handler
    avcEventHandlerRef = le_avdata_AddSessionStateHandler(AvcStatusHandler, NULL);
    // Request AVC session. Note: AVC handler must be registered prior starting a session
    le_avdata_RequestSessionObjRef_t sessionRequestRef = le_avdata_RequestSession();
    if (NULL == sessionRequestRef)
    {
        LE_ERROR("AirVantage Connection Controller does not start.");
    }
    else
    {
        sessionRef=sessionRequestRef;
        LE_INFO("AirVantage Connection Controller started.");
    }

    LE_INFO("Started LWM2M session with AirVantage");

    // Create resources
    LE_INFO("Create instances AssetData ");

    for ( i = 0; i < NUM_ARRAY_MEMBERS(ConfigEntries); i++)
    {
        resultCreateResources = le_avdata_CreateResource(ConfigEntries[i].configTreePathPtr, LE_AVDATA_ACCESS_SETTING);
        if (LE_FAULT == resultCreateResources)
        {
            LE_ERROR("Error in creating %s", ConfigEntries[i].configTreePathPtr);
        }
    }

    // Register handler for Write Settings
    LE_INFO("Register handler of paths");

    for ( i = 0; i < NUM_ARRAY_MEMBERS(ConfigEntries); i++)
    {
        le_avdata_AddResourceEventHandler(ConfigEntries[i].configTreePathPtr, ConfigSettingHandler, ConfigEntries[i].resourcePathPtr);
    }
}