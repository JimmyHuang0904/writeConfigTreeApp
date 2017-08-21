#include "legato.h"
#include "interfaces.h"

//-------------------------------------------------------------------------------------------------
/**
 * Declare asset data path
 */
//-------------------------------------------------------------------------------------------------

/* variables */
// string - room name
#define ROOM_NAME_VAR_RES   "/home1/room1/roomName"

// bool - status of air conditioning in the room ON or OFF
#define IS_AC_ON_VAR_RES   "/home1/room1/AC/IsACOn"

// float - room temperature reading
#define ROOM_TEMP_READING_VAR_RES "/home1/room1/thermostat/roomTemp"

/* settings*/

//float - target temperature setting
#define TARGET_TEMP_SET_RES "/home1/room1/thermostat/targetTemp"

/* commands */

// commands to turn off the air conditioning
#define AC_CMD_TURN_OFF_RES              "/home1/room1/AC/ACControl"

//-------------------------------------------------------------------------------------------------
/**
 * AVC related variable and update timer
 */
//-------------------------------------------------------------------------------------------------
// reference timer for app session
le_timer_Ref_t sessionTimer;
//reference to AVC event handler
le_avdata_SessionStateHandlerRef_t  avcEventHandlerRef = NULL;
//reference to AVC Session handler
le_avdata_RequestSessionObjRef_t sessionRef = NULL;
//reference to temperature update timer
le_timer_Ref_t tempUpdateTimerRef = NULL;
//reference to push asset data timer
le_timer_Ref_t serverUpdateTimerRef = NULL;

//-------------------------------------------------------------------------------------------------
/**
 * Counters recording the number of times certain hardwares are accessed.
 */
//-------------------------------------------------------------------------------------------------
static int WriteTempSettingCounter = 0;
static int ExecACCmd = 0;

//-------------------------------------------------------------------------------------------------
/**
 * Target temperature related declarations.
 */
//-------------------------------------------------------------------------------------------------
static char* RoomNameVar;
static double RoomTempVar = 0.0;
static int TargetTempSet = 0;
static bool IsACOn = false;


//-------------------------------------------------------------------------------------------------
/**
 * Asset data handlers
 */
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/**
 * Setting data handler.
 * This function is returned whenever AirVantage performs a read or write on the target temperature
 */
//-------------------------------------------------------------------------------------------------
static void TempSettingHandler
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    WriteTempSettingCounter++;

    LE_INFO("------------------- Server writes temperature setting [%d] times ------------",
                WriteTempSettingCounter);
    int targetTempLatest = 0;
    le_result_t resultGetInt = le_avdata_GetInt(TARGET_TEMP_SET_RES, &targetTempLatest);
    if (LE_FAULT == resultGetInt)
    {
        LE_ERROR("Error in getting latest TARGET_TEMP_SET_RES");
    }
    // if targetTempLatest from server is not the same as the current TargetTempSet
    // ,then set to the latest one

    if (targetTempLatest != TargetTempSet){
        TargetTempSet=targetTempLatest;
        le_result_t resultSetTargetTemp = le_avdata_SetInt(TARGET_TEMP_SET_RES, TargetTempSet);
        if (LE_FAULT == resultSetTargetTemp)
        {
            LE_ERROR("Error in getting latest TARGET_TEMP_SET_RES");
        }
        LE_INFO("Setting Write target temperature request: %s is %d C°",
                                            "TARGET_TEMP_SET_RES", TargetTempSet);
    }
    // turn on the air conditioning if room temperature is higher than target temperature
    if (TargetTempSet < RoomTempVar)
    {
        le_result_t resultSetAC = le_avdata_SetBool(IS_AC_ON_VAR_RES, true);
        if (LE_FAULT == resultSetAC)
        {
            LE_ERROR("Error in setting IS_AC_ON_VAR_RES");
        }
        LE_INFO("Setting Write turning on AC variable request: %s", "IS_AC_ON_VAR_RES");
    }else{
        le_result_t resultSetAC = le_avdata_SetBool(IS_AC_ON_VAR_RES, true);
        if (LE_FAULT == resultSetAC)
        {
            LE_ERROR("Error in setting IS_AC_ON_VAR_RES");
        }
        LE_INFO("Setting Write turning on AC variable request: %s", "IS_AC_ON_VAR_RES");
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Command data handler.
 * This function is returned whenever AirVantage performs an execute on the AC turn off command
 */
//-------------------------------------------------------------------------------------------------
static void ExecACCtrlCmd
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    ExecACCmd++;
    LE_INFO("------------------- Exec AC Commnad [%d] times ------------",
                ExecACCmd);
    le_result_t setACVAR = le_avdata_SetBool(IS_AC_ON_VAR_RES,false);
    if (LE_FAULT == setACVAR)
    {
        LE_ERROR("Error in setting IS_AC_ON_VAR_RES");
    }
    LE_INFO("Command exec turning off AC variable request: %s", "IS_AC_ON_VAR_RES");
}

double ConvergeTemperature(double currentTemperature, int targetTemperature)
{
    double fTargetTemp = (double) targetTemperature;

    if (currentTemperature == fTargetTemp)
    {
        return fTargetTemp;
    }

    double fStep = 0.2;

    if (currentTemperature > fTargetTemp)
    {
        currentTemperature -= fStep;
    }
    else
    {
        currentTemperature += fStep;
    }

    return currentTemperature;
}

//-------------------------------------------------------------------------------------------------
/**
 * Asset data push
 */
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/**
 * Push ack callback handler
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
            LE_INFO("Legato assetdata push successfully");
            break;
        case LE_AVDATA_PUSH_FAILED:
            LE_INFO("Legato assetdata push failed");
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
        le_result_t resultPushRoomName;
        resultPushRoomName = le_avdata_Push(ROOM_NAME_VAR_RES, PushCallbackHandler, NULL);
        if (LE_FAULT == resultPushRoomName)
        {
            LE_ERROR("Error pushing ROOM_NAME_VAR_RES");
        }
        le_result_t resultPushACStatus;
        resultPushACStatus = le_avdata_Push(IS_AC_ON_VAR_RES, PushCallbackHandler, NULL);
        if (LE_FAULT == resultPushACStatus)
        {
            LE_ERROR("Error pushing IS_AC_ON_VAR_RES");
        }
        le_result_t resultPushRoomTemp;
        resultPushRoomTemp = le_avdata_Push(ROOM_TEMP_READING_VAR_RES, PushCallbackHandler, NULL);
        if (LE_FAULT == resultPushRoomTemp)
        {
            LE_ERROR("Error pushing ROOM_TEMP_READING_VAR_RES");
        }
        le_result_t resultPushTargetTemp;
        resultPushTargetTemp = le_avdata_Push(TARGET_TEMP_SET_RES, PushCallbackHandler, NULL);
        if (LE_FAULT == resultPushTargetTemp)
        {
            LE_ERROR("Error pushing TARGET_TEMP_SET_RES");
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

    // Create resources
    LE_INFO("Create instances AssetData ");
    le_result_t resultCreateRoomName;
    resultCreateRoomName = le_avdata_CreateResource(ROOM_NAME_VAR_RES,LE_AVDATA_ACCESS_VARIABLE);
    if (LE_FAULT == resultCreateRoomName)
    {
        LE_ERROR("Error in creating ROOM_NAME_VAR_RES");
    }
    le_result_t resultCreateIsACOn;
    resultCreateIsACOn = le_avdata_CreateResource(IS_AC_ON_VAR_RES,LE_AVDATA_ACCESS_VARIABLE);
    if (LE_FAULT == resultCreateIsACOn)
    {
        LE_ERROR("Error in creating IS_AC_ON_VAR_RES");
    }
    le_result_t resultCreateRoomTemp;
    resultCreateRoomTemp = le_avdata_CreateResource(ROOM_TEMP_READING_VAR_RES,LE_AVDATA_ACCESS_VARIABLE);
    if (LE_FAULT == resultCreateRoomTemp)
    {
        LE_ERROR("Error in creating ROOM_TEMP_READING_VAR_RES");
    }

    le_result_t resultCreateTargetTemp;
    resultCreateTargetTemp = le_avdata_CreateResource(TARGET_TEMP_SET_RES,LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateTargetTemp)
    {
        LE_ERROR("Error in creating TARGET_TEMP_SET_RES");
    }

    le_result_t resultCreateACCmd = le_avdata_CreateResource(AC_CMD_TURN_OFF_RES, LE_AVDATA_ACCESS_COMMAND);
    if (LE_FAULT == resultCreateACCmd)
    {
        LE_ERROR("Error in creating AC_CMD_TURN_OFF_RES");
    }

    //setting the variable initial value
    TargetTempSet = 21;
    RoomTempVar = 30.0;
    RoomNameVar = "Room1";
    le_result_t resultSetRoomName = le_avdata_SetString(ROOM_NAME_VAR_RES, RoomNameVar);
    if (LE_FAULT == resultSetRoomName)
    {
        LE_ERROR("Error in setting ROOM_NAME_VAR_RES");
    }
    le_result_t resultSetIsACOn = le_avdata_SetBool(IS_AC_ON_VAR_RES, IsACOn);
    if (LE_FAULT == resultSetIsACOn)
    {
        LE_ERROR("Error in setting IS_AC_ON_VAR_RES");
    }
    le_result_t resultSetRoomTemp = le_avdata_SetFloat(ROOM_TEMP_READING_VAR_RES,RoomTempVar);
    if (LE_FAULT == resultSetRoomTemp)
    {
        LE_ERROR("Error in setting ROOM_TEMP_READING_VAR_RES");
    }
    le_result_t resultSetTargetTemp = le_avdata_SetInt(TARGET_TEMP_SET_RES,TargetTempSet);
    if (LE_FAULT == resultSetTargetTemp)
    {
        LE_ERROR("Error in setting TARGET_TEMP_SET_RES");
    }

    //Register handler for Variables, Settings and Commands
    LE_INFO("Register handler of paths");

    le_avdata_AddResourceEventHandler(TARGET_TEMP_SET_RES,
                                      TempSettingHandler, NULL);

    le_avdata_AddResourceEventHandler(AC_CMD_TURN_OFF_RES, ExecACCtrlCmd, NULL);

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
