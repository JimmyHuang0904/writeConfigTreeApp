#include "legato.h"
#include "interfaces.h"

//#define APP_RUNNING_DURATION_SEC 110        //run this app for 10min
#define ARRAY_SIZE 512

/* settings*/

//float - target temperature setting
#define CONFIG_TREE_NAME "trafficLight:"
#define CONFIG_TREE_URL "/url"
//#define CONFIG_TREE_INFO_EXITCODE_CHECKFLAG "/info/exitCode/checkFlag"
#define CONFIG_TREE_POLLINGINTERVALSEC "/pollingIntervalSec"

//-------------------------------------------------------------------------------------------------
/**
 * AVC related variable and update timer
 */
//-------------------------------------------------------------------------------------------------
// reference timer for app sessionTimer
le_timer_Ref_t sessionTimer;
//reference to AVC event handler
le_avdata_SessionStateHandlerRef_t  avcEventHandlerRef = NULL;
//reference to AVC Session handler
le_avdata_RequestSessionObjRef_t sessionRef = NULL;
//reference to push asset data timer
//le_timer_Ref_t serverUpdateTimerRef = NULL;

//-------------------------------------------------------------------------------------------------
/**
 * Asset data handlers
 */
//-------------------------------------------------------------------------------------------------

static void writeConfig
(
    char * dataToWrite,
    char * pathToData
)
{
    le_cfg_IteratorRef_t iteratorRef;
    // Set default Url to the global variable Url
    iteratorRef = le_cfg_CreateWriteTxn(CONFIG_TREE_NAME);
    le_cfg_SetString(iteratorRef, pathToData, dataToWrite);
    le_cfg_CommitTxn(iteratorRef);
}
//-------------------------------------------------------------------------------------------------
/**
 * Setting data handler.
 * This function is returned whenever AirVantage performs a read or write on the target temperature
 */
//-------------------------------------------------------------------------------------------------
static void UrlSettingHandler
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    char * pathToData;

    if ( !strcmp( (char *) contextPtr, "urlPtr") )
    {
        LE_INFO("I am in context ptr url");
        pathToData = CONFIG_TREE_URL;
        LE_INFO(" path to data is %s", pathToData);
    }
    else if ( !strcmp( (char *) contextPtr, "pollingIntervalSecPtr") )
    {
        LE_INFO("I am in pollingIntervalSecPtr");
        pathToData = CONFIG_TREE_POLLINGINTERVALSEC;
        LE_INFO(" path to data is %s", pathToData);
    }

    LE_INFO("------------------- Server writes to: %s ------------------", (char *) contextPtr);
    char bufferDataLatest[ARRAY_SIZE] = "";

    le_result_t resultGetString = le_avdata_GetString(CONFIG_TREE_URL, bufferDataLatest, ARRAY_SIZE);
    if (LE_FAULT == resultGetString)
    {
        LE_ERROR("Error in getting latest CONFIG_TREE_URL");
    }
    le_result_t resultSetTargetTemp = le_avdata_SetString(CONFIG_TREE_URL, bufferDataLatest);
    if (LE_FAULT == resultSetTargetTemp)
    {
        LE_ERROR("Error in getting latest CONFIG_TREE_URL");
    }

    writeConfig(bufferDataLatest, pathToData);
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
    else{
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

    le_result_t resultSetTargetTemp = le_avdata_SetString(CONFIG_TREE_URL, "");
    if (LE_FAULT == resultSetTargetTemp)
    {
        LE_ERROR("Error in setting CONFIG_TREE_URL");
    }

    le_result_t resultCreatePollingSec;
    resultCreatePollingSec = le_avdata_CreateResource(CONFIG_TREE_POLLINGINTERVALSEC, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreatePollingSec)
    {
        LE_ERROR("Error in creating CONFIG_TREE_POLLINGINTERVALSEC");
    }

    // le_result_t resultSetTargetTemp = le_avdata_SetInt(CONFIG_TREE_POLLINGINTERVALSEC, DEFA);
    // if (LE_FAULT == resultSetTargetTemp)
    // {
    //     LE_ERROR("Error in setting CONFIG_TREE_URL");
    // }



    //Register handler for Variables, Settings and Commands
    LE_INFO("Register handler of paths");

    char * urlPtr = "urlPtr";
    char * pollingIntervalSecPtr = "pollingIntervalSecPtr";
    le_avdata_AddResourceEventHandler(CONFIG_TREE_URL, UrlSettingHandler, urlPtr);
    le_avdata_AddResourceEventHandler(CONFIG_TREE_POLLINGINTERVALSEC, UrlSettingHandler, pollingIntervalSecPtr);
}
