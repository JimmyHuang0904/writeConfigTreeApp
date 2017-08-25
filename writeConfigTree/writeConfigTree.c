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

// Define contextPtr tags for each path
#define URL_PTR "urlPtr"
#define EXITCODE_CHECKFLAG_PTR "exitCodeFlagPtr"
#define CONTENT_CHECKFLAG_PTR "contentFlagPtr"
#define POLLINGINTERVALSEC_PTR "pollingIntervalSecPtr"

//-------------------------------------------------------------------------------------------------
/**
 * AVC related variable
 */
//-------------------------------------------------------------------------------------------------
//reference to AVC event handler
le_avdata_SessionStateHandlerRef_t  avcEventHandlerRef = NULL;
//reference to AVC Session handler
le_avdata_RequestSessionObjRef_t sessionRef = NULL;

//-------------------------------------------------------------------------------------------------
/**
 * Functions to write different data types (bool, string, int) to the config tree given the
 * path and the data
 */
//-------------------------------------------------------------------------------------------------
static void writeBoolToConfig
(
    bool dataToWrite,
    char * pathToData
)
{
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateWriteTxn(CONFIG_TREE_NAME);
    le_cfg_SetBool(iteratorRef, pathToData, dataToWrite);
    le_cfg_CommitTxn(iteratorRef);
}

static void writeStringToConfig
(
    char * dataToWrite,
    char * pathToData
)
{
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateWriteTxn(CONFIG_TREE_NAME);
    le_cfg_SetString(iteratorRef, pathToData, dataToWrite);
    le_cfg_CommitTxn(iteratorRef);
}

static void writeIntToConfig
(
    int dataToWrite,
    char * pathToData
)
{
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateWriteTxn(CONFIG_TREE_NAME);
    le_cfg_SetInt(iteratorRef, pathToData, dataToWrite);
    le_cfg_CommitTxn(iteratorRef);
}
//-------------------------------------------------------------------------------------------------
/**
 * This function is returned whenever a write operation occurs
 * 
 */
//-------------------------------------------------------------------------------------------------
static void configSettingHandler
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    char * pathToData;
    char * dataType = "null";

    char bufferString[ARRAY_SIZE] = "";
    int bufferInt;
    bool bufferBool;

    // find the pathToData and the dataType through the contextPtr sent by the handler.
    // add more if cases to handle more paths
    if ( !strcmp( (char *) contextPtr, URL_PTR) )
    {
        pathToData = CONFIG_TREE_URL;
        dataType = "string";
    }
    else if ( !strcmp( (char *) contextPtr, POLLINGINTERVALSEC_PTR) )
    {
        pathToData = CONFIG_TREE_POLLINGINTERVALSEC;
        dataType = "int";
    }
    else if ( !strcmp( (char *) contextPtr, EXITCODE_CHECKFLAG_PTR) )
    {
        pathToData = CONFIG_TREE_INFO_EXITCODE_CHECKFLAG;
        dataType = "bool";
    }
    else if ( !strcmp( (char *) contextPtr, CONTENT_CHECKFLAG_PTR) )
    {
        pathToData = CONFIG_TREE_INFO_CONTENT_CHECKFLAG;
        dataType = "bool";
    }
    else
    {
        LE_INFO("Nothing to Write");
        return;
    }

    LE_INFO("------------- Server writes to: %s -------------", (char *) contextPtr);
    LE_INFO("dataType is : %s, path to data is : %s", dataType, pathToData);


    if ( !strcmp( dataType, "string" ) )
    {
        le_result_t resultGetString = le_avdata_GetString(pathToData, bufferString, ARRAY_SIZE);
        if (LE_FAULT == resultGetString)
        {
            LE_ERROR("Error in getting latest %s", pathToData);
        }
        le_result_t resultSetString = le_avdata_SetString(pathToData, bufferString);
        if (LE_FAULT == resultSetString)
        {
            LE_ERROR("Error in getting latest %s", pathToData);
        }

        writeStringToConfig(bufferString, pathToData);
    }
    else if ( !strcmp( dataType, "int" ) )
    {
        LE_INFO("pathToData is %s, bufferInt before getint is %i (should be 10)", pathToData, bufferInt);

        le_result_t resultGetInt = le_avdata_GetInt(pathToData, &bufferInt);
        if (LE_FAULT == resultGetInt)
        {
            LE_ERROR("Error in getting latest %s", pathToData);
        }
        LE_INFO(" buffer int is =%i, should be what is written on myAVLib", bufferInt);
        le_result_t resultSetInt = le_avdata_SetInt(pathToData, bufferInt);
        if (LE_FAULT == resultSetInt)
        {
            LE_ERROR("Error in getting latest %s", pathToData);
        }

        writeIntToConfig(bufferInt, pathToData);
    }
    else if ( !strcmp( dataType, "bool" ) )
    {
        le_result_t resultGetBool = le_avdata_GetBool(pathToData, &bufferBool);
        if (LE_FAULT == resultGetBool)
        {
            LE_ERROR("Error in getting latest %s", pathToData);
        }
        le_result_t resultSetBool = le_avdata_SetBool(pathToData, bufferBool);
        if (LE_FAULT == resultSetBool)
        {
            LE_ERROR("Error in getting latest %s", pathToData);
        }

        writeBoolToConfig(bufferBool, pathToData);
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
static void sig_appTermination_cbh(int sigNum)
{
    LE_INFO("Close AVC session");
    le_avdata_ReleaseSession(sessionRef);
    if (NULL != avcEventHandlerRef)
    {
        //unregister the handler
        LE_INFO("Unregister the session handler");
        le_avdata_RemoveSessionStateHandler(avcEventHandlerRef);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Status handler for avcService updates
 */
//-------------------------------------------------------------------------------------------------
static void avcStatusHandler
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
    }
}

COMPONENT_INIT
{
    LE_INFO("Start Legato writeConfigTree App");

    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, sig_appTermination_cbh);

    //Start AVC Session
    //Register AVC handler
    avcEventHandlerRef = le_avdata_AddSessionStateHandler(avcStatusHandler, NULL);
    //Request AVC session. Note: AVC handler must be registered prior starting a session
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

    le_result_t resultCreateUrl;
    resultCreateUrl = le_avdata_CreateResource(CONFIG_TREE_URL, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateUrl)
    {
        LE_ERROR("Error in creating CONFIG_TREE_URL");
    }

    le_result_t resultCreatePollingSec;
    resultCreatePollingSec = le_avdata_CreateResource(CONFIG_TREE_POLLINGINTERVALSEC, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreatePollingSec)
    {
        LE_ERROR("Error in creating CONFIG_TREE_POLLINGINTERVALSEC");
    }

    le_result_t resultExitCodeCheck;
    resultExitCodeCheck = le_avdata_CreateResource(CONFIG_TREE_INFO_EXITCODE_CHECKFLAG, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultExitCodeCheck)
    {
        LE_ERROR("Error in creating CONFIG_TREE_INFO_EXITCODE_CHECKFLAG");
    }

    le_result_t resultContentCheck;
    resultContentCheck = le_avdata_CreateResource(CONFIG_TREE_INFO_CONTENT_CHECKFLAG, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultContentCheck)
    {
        LE_ERROR("Error in creating CONFIG_TREE_INFO_CONTENT_CHECKFLAG");
    }

    // Initialize Data
    le_result_t resultSetPollingSec = le_avdata_SetInt(CONFIG_TREE_POLLINGINTERVALSEC, 20);
    if (LE_FAULT == resultSetPollingSec)
    {
        LE_ERROR("Error in setting CONFIG_TREE_POLLINGINTERVALSEC");
    }

    le_result_t resultSetUrl = le_avdata_SetString(CONFIG_TREE_URL, "");
    if (LE_FAULT == resultSetUrl)
    {
        LE_ERROR("Error in setting CONFIG_TREE_URL");
    }

    // Register handler for Write Settings
    LE_INFO("Register handler of paths");

    le_avdata_AddResourceEventHandler(CONFIG_TREE_URL, configSettingHandler, URL_PTR);
    le_avdata_AddResourceEventHandler(CONFIG_TREE_POLLINGINTERVALSEC, configSettingHandler, POLLINGINTERVALSEC_PTR);
    le_avdata_AddResourceEventHandler(CONFIG_TREE_INFO_EXITCODE_CHECKFLAG, configSettingHandler, EXITCODE_CHECKFLAG_PTR);
    le_avdata_AddResourceEventHandler(CONFIG_TREE_INFO_CONTENT_CHECKFLAG, configSettingHandler, CONTENT_CHECKFLAG_PTR);
}