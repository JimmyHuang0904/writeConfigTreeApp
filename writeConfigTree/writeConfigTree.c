#include "legato.h"
#include "interfaces.h"

#define APP_RUNNING_DURATION_SEC 110        //run this app for 10min
#define ARRAY_SIZE 512

/* settings*/

//float - target temperature setting
#define URL_SET "/trafficLight/url"

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
le_timer_Ref_t serverUpdateTimerRef = NULL;

//-------------------------------------------------------------------------------------------------
/**
 * Asset data handlers
 */
//-------------------------------------------------------------------------------------------------

static void writeConfig
(
    char * url
)
{
    le_cfg_IteratorRef_t iteratorRef;
    // Set default Url to the global variable Url
    iteratorRef = le_cfg_CreateWriteTxn("trafficLight:/");
    le_cfg_SetString(iteratorRef, "url", url);
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
    // WriteUrlSettingCounter++;

    // LE_INFO("------------------- Server writes temperature setting [%d] times ------------",
    //             WriteUrlSettingCounter);
    char targetTempLatest[ARRAY_SIZE] = "";

    le_result_t resultGetString = le_avdata_GetString(URL_SET, targetTempLatest, ARRAY_SIZE);
    if (LE_FAULT == resultGetString)
    {
        LE_ERROR("Error in getting latest URL_SET");
    }
    le_result_t resultSetTargetTemp = le_avdata_SetString(URL_SET, targetTempLatest);
    if (LE_FAULT == resultSetTargetTemp)
    {
        LE_ERROR("Error in getting latest URL_SET");
    }

    writeConfig(targetTempLatest);
}

//-------------------------------------------------------------------------------------------------
/**
 * Asset data push
 */
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/**
 * This function is called whenever push has been performed successfully in AirVantage server
 */
//-------------------------------------------------------------------------------------------------
static void PushCallbackHandler
(
    le_avdata_PushStatus_t status,
    void* contextPtr
)
{
    switch (status)
    {
        case LE_AVDATA_PUSH_SUCCESS:
            LE_INFO("Legato writeConfigTree push successfully");
            break;
        case LE_AVDATA_PUSH_FAILED:
            LE_INFO("Legato writeConfigTree push failed");
            break;
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Push ack callback handler
 * This function is called every 10 seconds to push the data and update data in AirVantage server
 */
//-------------------------------------------------------------------------------------------------
void PushResources(le_timer_Ref_t  timerRef)
{
    // if session is still open, push the values
    if (NULL != avcEventHandlerRef)
    {
        le_result_t resultPushTargetTemp;
        resultPushTargetTemp = le_avdata_Push(URL_SET, PushCallbackHandler, NULL);
        if (LE_FAULT == resultPushTargetTemp)
        {
            LE_ERROR("Error pushing URL_SET");
        }
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

static void timerExpiredHandler(le_timer_Ref_t  timerRef)
{
    sig_appTermination_cbh(0);

    LE_INFO("Legato writeConfigTree App Ended");

    //Quit the app
    exit(EXIT_SUCCESS);
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
    }else{
        sessionRef=sessionRequestRef;
        LE_INFO("AirVantage Connection Controller started.");
    }

    LE_INFO("Started LWM2M session with AirVantage");
    sessionTimer = le_timer_Create("AssetDataAppSessionTimer");
    le_clk_Time_t avcInterval = {APP_RUNNING_DURATION_SEC, 0};
    le_timer_SetInterval(sessionTimer, avcInterval);
    le_timer_SetRepeat(sessionTimer, 1);
    le_timer_SetHandler(sessionTimer, timerExpiredHandler);
    le_timer_Start(sessionTimer);

    // Create resources
    LE_INFO("Create instances AssetData ");

    le_result_t resultCreateTargetTemp;
    resultCreateTargetTemp = le_avdata_CreateResource(URL_SET,LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateTargetTemp)
    {
        LE_ERROR("Error in creating URL_SET");
    }

    le_result_t resultSetTargetTemp = le_avdata_SetString(URL_SET,"");
    if (LE_FAULT == resultSetTargetTemp)
    {
        LE_ERROR("Error in setting URL_SET");
    }

    //Register handler for Variables, Settings and Commands
    LE_INFO("Register handler of paths");

    le_avdata_AddResourceEventHandler(URL_SET, UrlSettingHandler, NULL);

    //Set timer to update on server on a regular basis
    serverUpdateTimerRef = le_timer_Create("serverUpdateTimer");     //create timer
    le_clk_Time_t serverUpdateInterval = { 10, 0 };            //update server every 10 seconds
    le_timer_SetInterval(serverUpdateTimerRef, serverUpdateInterval);
    le_timer_SetRepeat(serverUpdateTimerRef, 0);                   //set repeat to always
    //set callback function to handle timer expiration event
    le_timer_SetHandler(serverUpdateTimerRef, PushResources);
    //start timer
    le_timer_Start(serverUpdateTimerRef);
}
