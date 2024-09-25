/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Sky
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Copyright [2014] [Cisco Systems, Inc.]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     LICENSE-2.0" target="_blank" rel="nofollow">http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ---- Include Files ---------------------------------------- */
#include <sys/un.h>
#include <errno.h>
#include <regex.h>
#include "pppmgr_global.h"
#include "pppmgr_data.h"
#include "pppmgr_dml.h"
#include <ipc_msg.h> 
#include "pppmgr_dml_ppp_apis.h"
#include "syscfg.h"

#define PPP_MGR_IPC_SERVER    1
#define PROC_UUID_PATH        "/proc/sys/kernel/random/uuid"

/* pppd exit status */
#define PPP_EXIT_USER_REQUEST        5
#define PPP_EXIT_PEER_AUTH_FAILED    11
#define PPP_EXIT_IDLE_TIMEOUT        12
#define PPP_EXIT_HANGUP              16
#define PPP_EXIT_AUTH_TOPEER_FAILED  19

#define PPP_EVENT_QUEUE_NAME "/pppmgr_queue"
#define MAX_QUEUE_LENGTH           100

/*-------------------declarations--------------------*/
int       sysevent_fd = -1;
token_t   sysevent_token;

/* ---- private Functions ------------------------------------ */
static ANSC_STATUS PppMgr_createIpcSockFd( int32_t  *sockFd, uint32_t sockMode);
static ANSC_STATUS  PppMgr_bindIpcSocket( int32_t sockFd);
static void* PppMgr_EventHandlerThread( void *arg );
static ANSC_STATUS PppMgr_IpcServerInit();
static ANSC_STATUS PppMgr_DmlSetIp4Param (char * ipbuff, char * ipCharArr);
static ANSC_STATUS PppMgr_ProcessStateChangedMsg(int InstanceNumber, ipc_ppp_event_msg_t pppEventMsg);
static ANSC_STATUS PppMgr_receiveIpcSocket(int32_t sockFd, char *msg, uint32_t *msgLen);
static ANSC_STATUS PppMgr_ProcessIpcpParams(int InstanceNumber, ipc_ppp_event_msg_t pppEventMsg);
static ANSC_STATUS PppMgr_ProcessIpv6cpParams(int InstanceNumber, ipc_ppp_event_msg_t pppEventMsg);
static ANSC_STATUS PppMgr_ProcessPppState(ipc_msg_payload_t ipcMsg);
static int PppMgr_ProcessPppEvent(PPPEventQData * eventMsg);
static ANSC_STATUS PppMgr_StartIpcServer();
/* ------------------extern variables -------------------------*/
extern PBACKEND_MANAGER_OBJECT               g_pBEManager;
extern  ANSC_HANDLE bus_handle;

/*-------------------public functions---------------------------------------*/
ANSC_STATUS PppMgr_createIpcSocket(int32_t *sockFd, uint32_t sockMode);

/* ---- Private Variables ------------------------------------ */
static int   ipcListenFd;   /* Unix domain IPC listening socket fd */
static char *pppStateNames[] =
{
    [PPP_INTERFACE_UP] = "PPP_INTERFACE_UP",
    [PPP_INTERFACE_DOWN] = "PPP_INTERFACE_DOWN",
    [PPP_INTERFACE_UNCONFIGURED] = "PPP_INTERFACE_UNCONFIGURED",
    [PPP_INTERFACE_CONNECTING]= "PPP_INTERFACE_CONNECTING",
    [PPP_INTERFACE_AUTHENTICATING] = "[PPP_INTERFACE_AUTHENTICATING",
    [PPP_INTERFACE_PENDING_DISCONNET] = "PPP_INTERFACE_PENDING_DISCONNET",
    [PPP_INTERFACE_DISCONNECTING] = "PPP_INTERFACE_DISCONNECTING",
    [PPP_INTERFACE_DISCONNECTED] = "PPP_INTERFACE_DISCONNECTED",
    [PPP_INTERFACE_LCP_ECHO_FAILED] = "PPP_INTERFACE_LCP_ECHO_FAILED",
    [PPP_INTERFACE_AUTH_FAILED] = "PPP_INTERFACE_AUTH_FAILED",
    [PPP_IPCP_COMPLETED] = "PPP_IPCP_COMPLETED",
    [PPP_IPCP_FAILED] = "PPP_IPCP_FAILED",
    [PPP_IPV6CP_COMPLETED] = "PPP_IPV6CP_COMPLETED",
    [PPP_IPV6CP_FAILED] = "PPP_IPV6CP_FAILED",
    [PPP_LCP_AUTH_COMPLETED] = "PPP_LCP_AUTH_COMPLETED",
    [PPP_MAX_STATE] = "PPP_MAX_STATE",
    [PPP_LCP_VENDOR_RECEIVED] = "PPP_LCP_VENDOR_RECEIVED"
};

/*-------------------Extern declarations--------------------*/
extern int PppManager_StartIpcServer();

ANSC_STATUS PppMgr_SendDataToQ (PPPEventQData * pEventData)
{
    if (pEventData == NULL)
    {
        CcspTraceError(("%s %d: invalid args\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d: Writing - action:%s PppIfInstance:%d keyPath=%s val=%s to queue\n", 
                __FUNCTION__, __LINE__, pEventData->action == PPPMGR_BUS_SET?"PPPMGR_BUS_SET":"PPPMGR_EXEC_PPP_CLIENT", pEventData->PppIfInstance, pEventData->keyPath, pEventData->val));

    mqd_t mq;

    mq = mq_open(PPP_EVENT_QUEUE_NAME, O_WRONLY);
    if (mq == -1)
    {
        CcspTraceError(("%s %d: mq_open failed :%s\n", __FUNCTION__, __LINE__, strerror(errno)));
        return ANSC_STATUS_FAILURE;
    }

    if (mq_send(mq, (const char *)pEventData, sizeof(PPPEventQData), 0) != 0 )
    {   
        CcspTraceError(("%s %d: mq_send failed :%s\n", __FUNCTION__, __LINE__, strerror(errno)));
        mq_close(mq);
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d: Successfully posted EvenetDate to Q\n", __FUNCTION__, __LINE__));

    mq_close(mq);
    return ANSC_STATUS_SUCCESS;
}
/* ---------------------------------------------------------------------------
   This internal API will convert state to string
----------------------------------------------------------------------------*/
static char* pppStatetoString(uint8_t state)
{
    if(state < PPP_MAX_STATE)
    {
        return pppStateNames[state];
    }
    return pppStateNames[PPP_MAX_STATE];
}



/* ---------------------------------------------------------------------------
   This internal API creates a socket descriptor based on mode. If mode is server,
   it used the NN_PUSH for NM socket . If mode is client it use the NN_PULL
   ----------------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_createIpcSockFd( int32_t  *sockFd, uint32_t sockMode )
{
#if defined(_USE_NM_MSG_SOCK)
    if(sockMode == PPP_MGR_IPC_SERVER)
    {
        *sockFd = nn_socket (AF_SP, NN_PULL);
    }
    else
    {
        *sockFd = nn_socket (AF_SP, NN_PUSH);
    }
#else
    //use normal TCP socket API to create server/client socket
#endif

    return (*sockFd >= 0 ? ANSC_STATUS_SUCCESS: ANSC_STATUS_FAILURE);
}

/* ------------------------------------------------------------
   This internal API bind the socket descriptot with a port number
   -------------------------------------------------------------*/
static ANSC_STATUS  PppMgr_bindIpcSocket( int32_t sockFd)
{

    char sockPort[BUFLEN_256] = { 0 };

    snprintf(sockPort, sizeof(sockPort), PPP_MANAGER_ADDR);

#if defined( _USE_NM_MSG_SOCK)
    if(nn_bind (sockFd, sockPort)  < 0)
    {
        CcspTraceError(("Error: nn_bind failed[%s] \n",nn_strerror(nn_errno ())));
        nn_close(sockFd);
        return ANSC_STATUS_FAILURE;
    }
    return ANSC_STATUS_SUCCESS;
#else
    return ANSC_STATUS_FAILURE;
#endif
}

/* --------------------------------------------------------------------
Function : PppMgr_createIpcSocket

Decription: This API creates a server or client socket based on mode.
If mode is server, it will create the socket bind to the the port number
If mode os client it will create the socket and connected to port number

-----------------------------------------------------------------------*/
extern ANSC_STATUS PppMgr_createIpcSocket(int32_t *sockFd, uint32_t sockMode)
{
    PppMgr_createIpcSockFd(sockFd, sockMode);

    if(sockMode == PPP_MGR_IPC_SERVER)
    {
        return PppMgr_bindIpcSocket(*sockFd);

    }
    return ANSC_STATUS_FAILURE;
}

/* --------------------------------------------------------------------
Function : PppMgr_receiveIpcSocket

Decription: This API receives message from scoket. Received message will be
copied to "msg" output varaible and the message length will be copied to
"msgLen" output variable

-----------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_receiveIpcSocket(int32_t sockFd, char *msg, uint32_t *msgLen)
{
    if(sockFd < 0 || msg == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

#ifdef _USE_NM_MSG_SOCK

    void *ipcBuff = NULL;
    *msgLen = nn_recv (sockFd, &ipcBuff, NN_MSG, NN_DONTWAIT);

    if(ipcBuff == NULL)
    {
        return ANSC_STATUS_FAILURE;
    }

    if( *msgLen > 0)
    {
        memcpy(msg, ipcBuff, *msgLen);
    }
    nn_freemsg (ipcBuff);
#endif

    return( *msgLen > 0 ? ANSC_STATUS_SUCCESS : ANSC_STATUS_FAILURE) ;
}

/* --------------------------------------------------------------------
Function : PppMgr_closeIpcSocket

Decription: This API will close the socket
-----------------------------------------------------------------------*/
extern ANSC_STATUS PppMgr_closeIpcSocket(int32_t sockFd)
{
#if defined(_USE_NM_MSG_SOCK)
    if(nn_shutdown (sockFd, 0) < 0)
    {
        return ANSC_STATUS_FAILURE;

    }
    return ANSC_STATUS_SUCCESS;
#else
    return ANSC_STATUS_FAILURE;
#endif
}


/* --------------------------------------------------------------------
Function : PppMgr_SetLinkStatusDown

Decription: This API will set down state to wan manager IPCP status
-----------------------------------------------------------------------*/
ANSC_STATUS PppMgr_SetLinkStatusDown(INT PppIfInstance)
{

    PPPEventQData eventData = {0};

    eventData.action = PPPMGR_BUS_SET; 
    eventData.PppIfInstance = PppIfInstance; 
    eventData.comPath = WAN_COMPONENT_NAME;
    eventData.busPath = WAN_DBUS_PATH;
    eventData.keyPath = PPP_LCP_STATUS_PARAM_NAME;
    strncpy(eventData.val, PPP_IPCP_STATUS_DOWN, sizeof(eventData.val) - 1);

    if (PppMgr_SendDataToQ(&eventData) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d - Failed to send data to Q\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}


/* --------------------------------------------------------------------
Function : PppMgr_SetIPCPStatusDown

Decription: This API will set down state to wan manager IPCP status
-----------------------------------------------------------------------*/
ANSC_STATUS PppMgr_SetIPCPStatusDown(INT PppIfInstance)
{

    PPPEventQData eventData = {0};

    eventData.action = PPPMGR_BUS_SET; 
    eventData.PppIfInstance = PppIfInstance; 
    eventData.comPath = WAN_COMPONENT_NAME;
    eventData.busPath = WAN_DBUS_PATH;
    eventData.keyPath = PPP_IPCP_STATUS_PARAM_NAME;
    strncpy(eventData.val, PPP_IPCP_STATUS_DOWN, sizeof(eventData.val) - 1);

    if (PppMgr_SendDataToQ(&eventData) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d - Failed to send data to Q\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}

/* --------------------------------------------------------------------
Function : PppMgr_SetIPv6CPStatusDown

Decription: This API will set down state to wan manager IPV6CP 
-----------------------------------------------------------------------*/
ANSC_STATUS PppMgr_SetIPv6CPStatusDown(INT PppIfInstance)
{
    PPPEventQData eventData = {0};

    eventData.action = PPPMGR_BUS_SET; 
    eventData.PppIfInstance = PppIfInstance; 
    eventData.comPath = WAN_COMPONENT_NAME;
    eventData.busPath = WAN_DBUS_PATH;
    eventData.keyPath = PPP_IPV6CP_STATUS_PARAM_NAME;
    strncpy(eventData.val, PPP_IPV6CP_STATUS_DOWN, sizeof(eventData.val) - 1);

    if (PppMgr_SendDataToQ(&eventData) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d - Failed to send data to Q\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;
}

/* --------------------------------------------------------------------
Function : PppMgr_StartIpcServer

Decription: This API will start the IPC server
-----------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_StartIpcServer()
{
    pthread_t ipcThreadId;
    int ret = 0;

    if(PppMgr_IpcServerInit() != ANSC_STATUS_SUCCESS)
    {
        CcspTraceInfo(("Failed to initialise IPC messaging"));

        return ANSC_STATUS_FAILURE;
    }

    ret = pthread_create( &ipcThreadId, NULL, &PppMgr_EventHandlerThread, NULL );

    if( 0 != ret )
    {
        CcspTraceInfo(("%s %d - Failed to start IPC Thread Error:%d\n", __FUNCTION__, __LINE__, ret));

        return ANSC_STATUS_FAILURE;
    }
    CcspTraceInfo(("%s %d - IPC Thread Started Successfully\n", __FUNCTION__, __LINE__));

    return ANSC_STATUS_SUCCESS ;
}

/* --------------------------------------------------------------------
Function : PppMgr_DmlSetIp4Param

Decription: This API will set IP parameters to data model
-----------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_DmlSetIp4Param (char * ipbuff, char * ipCharArr)
{
    char * s1 = NULL;
    char * s2 = NULL;
    int i = 0;

    s1 = ipbuff;

    while ((s2 = strtok(s1, ".")) != NULL)
    {
        if (i == IPV4_ADDRESS_SIZE)
        {
            printf("Incorrect IP\n");
            return ANSC_STATUS_FAILURE;
        }
        ipCharArr[i++] = atoi(s2);
        s1 = NULL;
    }

    return ANSC_STATUS_SUCCESS;
}

/* --------------------------------------------------------------------
Function : PppMgr_DmlSetVendorParams

Decription: This API will set vendor parameters to data model
-----------------------------------------------------------------------*/

static ANSC_STATUS PppMgr_DmlSetVendorParams(char *invendormsg , int *SRU , int *SRD)
{
    char * source = NULL;
    source = invendormsg;
    char * regexString = "([A-Z]+)=([0-9]+)";
    size_t maxMatches = 2; // No. of Strings matching pattern to extract
    size_t maxGroups = 1;

    regex_t regexCompiled;
    regmatch_t groupArray[maxGroups];
    unsigned int m;
    char * cursor;

    if(regcomp(&regexCompiled, regexString, REG_EXTENDED))
    {
        CcspTraceInfo(("%s %d Could not compile regex \n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    m = 0;
    cursor = source; // pointing to beginning of string
    for(m = 0; m < maxMatches; m ++)
    {
        if(regexec(&regexCompiled, cursor, maxGroups, groupArray, 0))
            break;  // No more matches

        unsigned int g = 0;
        unsigned int offset = 0;
        for(g = 0; g < maxGroups; g++)
        {
            if(groupArray[g].rm_so == (size_t)-1)
                break;  // No more groups

            if(g == 0)
                offset = groupArray[g].rm_eo;

            char cursorCopy[strlen(cursor) + 1];
            memset(cursorCopy, 0, sizeof(cursorCopy)); 
            strncpy(cursorCopy, cursor, sizeof(cursorCopy) - 1);
            cursorCopy[groupArray[g].rm_eo] = 0;
            char *ret;
            if((ret = strstr(cursorCopy + groupArray[g].rm_so, "SRU=")))
            {
                *SRU = strtol(ret+4, NULL, 10);
            }
            else if((ret = strstr(cursorCopy + groupArray[g].rm_so, "SRD=")))
            {
                *SRD = strtol(ret+4, NULL, 10);
            }
        }
        cursor += offset;
    }
    regfree(&regexCompiled);

    return ANSC_STATUS_SUCCESS;
}

/* --------------------------------------------------------------------
Function : PppMgr_SetLCPParams

Decription: This API will set SRU and SRD  when LCP state change message is received
-----------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_SetLCPParams(PDML_PPP_IF_FULL pNewEntry, ipc_ppp_event_msg_t pppEventMsg)
{
    int ret = 0;
    ULONG oldSRUvalue = pNewEntry->Info.SRU;
    ULONG oldSRDvalue = pNewEntry->Info.SRD;

    pNewEntry->Info.SRU = 0;
    pNewEntry->Info.SRD = 0;
    if(strlen(pppEventMsg.event.pppLcpMsg.vendormsg) > 0)
    {
        ret = PppMgr_DmlSetVendorParams(pppEventMsg.event.pppLcpMsg.vendormsg,(int *)&pNewEntry->Info.SRU, (int *)&pNewEntry->Info.SRD);
        if(ret == ANSC_STATUS_FAILURE)
        {
            CcspTraceError(("[%s-%d] Setting Vendor Params Falure%s\n", __FUNCTION__, __LINE__,pppEventMsg.event.pppLcpMsg.vendormsg));
        }
    }

#if defined(_DT_QoS_Enable_)
    FILE *fileExist;
    fileExist = fopen("/usr/sbin/set_qos.sh", "r");
    if (fileExist)
    {
        if (pNewEntry->Info.SRU != NULL)
        {
            char buf[56] = {0};
            snprintf(buf, sizeof(buf), "/usr/sbin/set_qos.sh %d", pNewEntry->Info.SRU);
            v_secure_system(buf);
            fclose(fileExist);
        }
        else
        {
            char buf[56] = {0};
            snprintf(buf, sizeof(buf), "/usr/sbin/set_qos.sh &");
            v_secure_system(buf);
            fclose(fileExist);
        }
    }
    else
    {
        CcspTraceInfo(("[%s-%d] script is not present\n", __FUNCTION__, __LINE__));
    }
#endif
    if( oldSRUvalue == pNewEntry->Info.SRU && oldSRDvalue == pNewEntry->Info.SRD )
    {
        CcspTraceInfo(("Internet connection was established with the download bandwidth: %d kbit/s and the upload bandwidth: %d kbit/s.\n",
                            pNewEntry->Info.SRD*1000,pNewEntry->Info.SRU*1000));
    }
    else
    {
        CcspTraceInfo(("Internet connection has been updated with download bandwidth: %d kbit/s and upload bandwidth: %d kbit/s.\n",
                            pNewEntry->Info.SRD*1000,pNewEntry->Info.SRU*1000));
    }
}

/* --------------------------------------------------------------------
Function : PppMgr_ProcessStateChangedMsg

Decription: This API will set ppp state when LCP state change message is received
-----------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_ProcessStateChangedMsg(int InstanceNumber, ipc_ppp_event_msg_t pppEventMsg)
{
    char WanPppLinkStatus[64] = { 0 };
    uint32_t updatedParam = 0;
    int ret = 0;
    char paramValue[256]   = {0};

    if(InstanceNumber <= 0 )
    {
        CcspTraceInfo(("[%s-%d] - Invalid instance number %d for pid %d\n", __FUNCTION__,
                    __LINE__, InstanceNumber, pppEventMsg.pid));

        return ANSC_STATUS_FAILURE;
    }
    CcspTraceInfo(("[%s-%d] - ipcp state for pid %d is %s\n", __FUNCTION__, __LINE__,
                pppEventMsg.pid, pppStatetoString(pppEventMsg.pppState)));

    PDML_PPP_IF_FULL pEntry = PppMgr_GetIfaceData_locked(InstanceNumber);

    if (pEntry != NULL)
    {

        switch(pppEventMsg.pppState)
        {
            case PPP_INTERFACE_UNCONFIGURED:
                pEntry->Info.Status = DML_IF_STATUS_Error;
                pEntry->Info.ConnectionStatus = DML_PPP_CONN_STATUS_Unconfigured;
                snprintf(WanPppLinkStatus, sizeof(WanPppLinkStatus), DOWN);
                updatedParam = 1;
                pEntry->Info.LastChange = GetUptimeinSeconds();
                break;

            case PPP_INTERFACE_CONNECTING:
                pEntry->Info.Status = DML_IF_STATUS_Down;
                pEntry->Info.ConnectionStatus = DML_PPP_CONN_STATUS_Connecting;
                pEntry->Info.LastChange = GetUptimeinSeconds();
                strncpy(pEntry->Cfg.ACName,pppEventMsg.event.pppLcpMsg.acname,sizeof(pEntry->Cfg.ACName));
                break;

            case PPP_INTERFACE_AUTHENTICATING:
                pEntry->Info.Status = DML_IF_STATUS_Down;
                pEntry->Info.ConnectionStatus = DML_PPP_CONN_STATUS_Authenticating;
                pEntry->Info.LastChange = GetUptimeinSeconds();
                break;

            case PPP_INTERFACE_UP:
                pEntry->Info.Status = DML_IF_STATUS_Up;
                pEntry->Info.ConnectionStatus = DML_PPP_CONN_STATUS_Connected;
                pEntry->Info.LastConnectionError = DML_PPP_CONN_ERROR_NONE;
                snprintf(WanPppLinkStatus, sizeof(WanPppLinkStatus), UP);
                updatedParam = 1;
                pEntry->Info.LastChange = GetUptimeinSeconds();

                CcspTraceInfo(("[%s-%d] Updating SessionID to PSM : %ld\n", __FUNCTION__, __LINE__, pEntry->Info.SessionID));
                get_session_id(&pEntry->Info.SessionID, pEntry);
                sprintf(paramValue, "%ld", pEntry->Info.SessionID);
                PppMgr_RdkBus_SetParamValuesToDB(PSM_PPP_SESSIONID, paramValue);
                break;

            case PPP_INTERFACE_DISCONNECTING:
                pEntry->Info.Status = DML_IF_STATUS_Down;
                pEntry->Info.ConnectionStatus = DML_PPP_CONN_STATUS_Disconnecting;
                pEntry->Info.LastChange = GetUptimeinSeconds();
                break;

            case PPP_INTERFACE_DISCONNECTED:
            case PPP_INTERFACE_DOWN:
                pEntry->Info.SRU = 0;
                pEntry->Info.SRD = 0;
                pEntry->Info.Status = DML_IF_STATUS_Down;
                pEntry->Info.ConnectionStatus = DML_PPP_CONN_STATUS_Disconnected;
                snprintf(WanPppLinkStatus, sizeof(WanPppLinkStatus), DOWN);
                updatedParam = 1;
                pEntry->Info.LastChange = GetUptimeinSeconds();

                switch(pppEventMsg.event.pppLcpMsg.exitStatus)
                {
                    case PPP_EXIT_USER_REQUEST:
                        pEntry->Info.LastConnectionError = DML_PPP_CONN_ERROR_USER_DISCONNECT;
                        break;
                    case PPP_EXIT_PEER_AUTH_FAILED:
                    case PPP_EXIT_AUTH_TOPEER_FAILED:
                        pEntry->Info.LastConnectionError = DML_PPP_CONN_ERROR_AUTHENTICATION_FAILURE;
                        break;
                    case PPP_EXIT_HANGUP:
                        pEntry->Info.LastConnectionError = DML_PPP_CONN_ERROR_ISP_TIME_OUT;
                        break;
                    default:
                        pEntry->Info.LastConnectionError = DML_PPP_CONN_ERROR_UNKNOWN;
                        break;
                }

                break;

            case PPP_LCP_VENDOR_RECEIVED:
                PppMgr_SetLCPParams(pEntry, pppEventMsg);
                break;

            case PPP_LCP_AUTH_COMPLETED:
                PppMgr_SetLCPParams(pEntry, pppEventMsg);
                if(strlen(pppEventMsg.event.pppLcpMsg.authproto) > 0)
                {
                    CcspTraceInfo(("PPP Authentication Protocol: %s ", pppEventMsg.event.pppLcpMsg.authproto));
                    if(strcmp(pppEventMsg.event.pppLcpMsg.authproto, "PAP") == 0)
                        pEntry->Info.AuthenticationProtocol = DML_PPP_AUTH_PAP;
                    else
                        pEntry->Info.AuthenticationProtocol = DML_PPP_AUTH_CHAP;
                }

                break;

            case PPP_INTERFACE_AUTH_FAILED:
                pEntry->Info.Status = DML_IF_STATUS_Down;
                pEntry->Info.ConnectionStatus = DML_PPP_CONN_STATUS_AuthenticationFailed;
                snprintf(WanPppLinkStatus, sizeof(WanPppLinkStatus), DOWN);
                PppMgr_stopPppoe();
                updatedParam = 1;
                break;

            default:
                pEntry->Info.Status = DML_IF_STATUS_Down;
                pEntry->Info.ConnectionStatus = DML_PPP_CONN_STATUS_Disconnected;
                snprintf(WanPppLinkStatus, sizeof(WanPppLinkStatus), DOWN);
                updatedParam = 1;
                pEntry->Info.LastChange = GetUptimeinSeconds();
                break;
        }
        /* We updated params in ppp data model . Update wan data model */
        if(!updatedParam)
        {
            /* We don't have an up/down status to update wan mananager */
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_SUCCESS;
        }
        /* Updating WanManager DM with PPP DML mutex lock creats mutex deadlock with WanManager DM mutex. Moving 
           DM set to thread to avoid mutex deadlock */ 
        PPPEventQData eventData = {0};

        eventData.action = PPPMGR_BUS_SET; 
        eventData.PppIfInstance = pEntry->Cfg.InstanceNumber; 
        eventData.comPath = WAN_COMPONENT_NAME;
        eventData.busPath = WAN_DBUS_PATH;
        eventData.keyPath = PPP_LCP_STATUS_PARAM_NAME;
        strncpy(eventData.val, WanPppLinkStatus, sizeof(eventData.val) - 1);

        if (PppMgr_SendDataToQ(&eventData) != ANSC_STATUS_SUCCESS)
        {
            CcspTraceError(("%s %d - Failed to send data to Q\n", __FUNCTION__, __LINE__));
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_FAILURE;
        }

        eventData.keyPath = PPP_LINK_STATUS_PARAM_NAME;
        if (PppMgr_SendDataToQ(&eventData) != ANSC_STATUS_SUCCESS)
        {
            CcspTraceError(("%s %d - Failed to send data to Q\n", __FUNCTION__, __LINE__));
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_FAILURE;
        }

        PppMgr_GetIfaceData_release(pEntry);
        return ANSC_STATUS_SUCCESS;
    }
    return ANSC_STATUS_FAILURE;
}

/* --------------------------------------------------------------------
Function : ProcessIpcpParams

Decription: This API will set IPV4 parameters to PPP data model 
-----------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_ProcessIpcpParams(int InstanceNumber, ipc_ppp_event_msg_t pppEventMsg)
{

    int ret = 0;
    char *s1 = NULL;
    int i = 0;
    char dns1[32] = { 0 };
    char dns2[32] = { 0 };
    int dnsCount = 0;

    if(InstanceNumber <= 0 )
    {
        CcspTraceError(("[%s-%d] - Invalid instance number %d for pid %d\n", __FUNCTION__,
                    __LINE__, InstanceNumber, pppEventMsg.pid));

        return ANSC_STATUS_FAILURE;
    }
    CcspTraceInfo((" %s %d: - ipcp state for pid %d is %s\n", __FUNCTION__, __LINE__,
                pppEventMsg.pid, pppStatetoString(pppEventMsg.pppState)));

    /* check incoming message for PPP IPCP complete state */
    if (pppEventMsg.pppState != PPP_IPCP_COMPLETED)
    {
        PppMgr_SetIPCPStatusDown(InstanceNumber);
        return ANSC_STATUS_FAILURE;
    }

    /* check network configuration parameters in incoming messages */
    if( (strcmp(pppEventMsg.event.pppIpcpMsg.ip, "") == 0 || 
                strcmp(pppEventMsg.event.pppIpcpMsg.gateway, "") == 0 ||
                strcmp(pppEventMsg.event.pppIpcpMsg.nameserver, "") == 0) )
    {
        CcspTraceInfo(("[%s-%d] Network parameters are missing from client message\n", __FUNCTION__, __LINE__));

        PppMgr_SetIPCPStatusDown(InstanceNumber);
        return ANSC_STATUS_FAILURE;
    }

    PDML_PPP_IF_FULL pEntry = PppMgr_GetIfaceData_locked(InstanceNumber);
    if (pEntry != NULL)
    {    

        /* Clear current data model values */
        memset (&pEntry->Info.LocalIPAddress, 0, sizeof(pEntry->Info.LocalIPAddress));
        memset (&pEntry->Info.RemoteIPAddress, 0, sizeof(pEntry->Info.RemoteIPAddress));
        memset (&pEntry->Info.DNSServers, 0, sizeof(pEntry->Info.DNSServers)); 

        ret = PppMgr_DmlSetIp4Param(pppEventMsg.event.pppIpcpMsg.ip, (char*)&pEntry->Info.LocalIPAddress);
        if (ret == ANSC_STATUS_FAILURE)
        {
            CcspTraceError(("%s %d: Setting Local IP Falure%s\n", __FUNCTION__, 
                        __LINE__, pppEventMsg.event.pppIpcpMsg.ip));

            PppMgr_SetIPCPStatusDown(pEntry->Cfg.InstanceNumber);
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_FAILURE;
        }

        ret = PppMgr_DmlSetIp4Param(pppEventMsg.event.pppIpcpMsg.gateway, (char*)&pEntry->Info.RemoteIPAddress);
        if (ret == ANSC_STATUS_FAILURE)
        {
            CcspTraceError(("[%s-%d] Setting Remote IP Falure%s\n", __FUNCTION__, __LINE__,
                        pppEventMsg.event.pppIpcpMsg.gateway));

            PppMgr_SetIPCPStatusDown(pEntry->Cfg.InstanceNumber);
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_FAILURE;
        }

        CcspTraceInfo(("[%s-%d] DNS received %s\n", __FUNCTION__, __LINE__, pppEventMsg.event.pppIpcpMsg.nameserver));
        s1 = pppEventMsg.event.pppIpcpMsg.nameserver;
        /* Parse DNS servers from message */
        for (i = 1, s1 = strtok(s1, ","); s1 != NULL; s1 = strtok(NULL, ","), i++)
        {
            dnsCount++;

            if(i > 1)
            {
                strncpy(dns2, s1, sizeof(dns2)-1);
                break;
            }
            strncpy(dns1, s1, sizeof(dns1)-1);
        }
        if(!dnsCount || dnsCount > 2)
        {
            CcspTraceInfo((" DNS parsing failed in received message\n"));

            PppMgr_SetIPCPStatusDown(pEntry->Cfg.InstanceNumber);
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_FAILURE;
        }
        /* Use a separate loop, otherwise strtok might fail */
        for(i = 1; i <= dnsCount ; i++)
        {
            CcspTraceInfo((" Updating DNS %d in data model\n", i ));

            if( i > 1)
            {
                PppMgr_DmlSetIp4Param(dns2, (char*)&pEntry->Info.DNSServers[i-1]);
                break;
            }
            PppMgr_DmlSetIp4Param(dns1, (char*)&pEntry->Info.DNSServers[i-1]);
        }

        PPPEventQData eventData = {0};
        eventData.PppIfInstance = pEntry->Cfg.InstanceNumber;
        eventData.comPath = WAN_COMPONENT_NAME;
        eventData.busPath = WAN_DBUS_PATH;
        eventData.keyPath = PPP_IPCP_STATUS_PARAM_NAME;
        strncpy(eventData.val, UP, sizeof(eventData.val) - 1);

        if (PppMgr_SendDataToQ(&eventData) != ANSC_STATUS_SUCCESS)
        {
            CcspTraceError(("%s %d - Failed to send data to Q\n", __FUNCTION__, __LINE__));
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_FAILURE;
        }

        PppMgr_GetIfaceData_release(pEntry);
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}
/* --------------------------------------------------------------------
Function : ProcessIpv6cpParams

Decription: This API will set IPV6 parameters to PPP data model 
-----------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_ProcessIpv6cpParams(int InstanceNumber, ipc_ppp_event_msg_t pppEventMsg)
{
    CcspTraceInfo(("[%s-%d] - instance number %d\n", __FUNCTION__, __LINE__, InstanceNumber));

    if(InstanceNumber <= 0 )
    {
        CcspTraceInfo(("[%s-%d] - Invalid instance number %d for pid %d\n", __FUNCTION__,
                    __LINE__, InstanceNumber, pppEventMsg.pid));

        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("[%s-%d] - IPV6CP state for pid %d is %s\n", __FUNCTION__, __LINE__,
                pppEventMsg.pid, pppStatetoString(pppEventMsg.pppState)));

    PDML_PPP_IF_FULL pEntry = PppMgr_GetIfaceData_locked(InstanceNumber);
    if (pEntry != NULL)
    {
        if (pppEventMsg.pppState == PPP_IPV6CP_COMPLETED)
        {
            if(strcmp(pppEventMsg.event.pppIpv6cpMsg.localIntfId , "") != 0)
            {
                memset(pEntry->Info.Ip6LocalIfID,0,sizeof(pEntry->Info.Ip6LocalIfID));		
                strncpy(pEntry->Info.Ip6LocalIfID, pppEventMsg.event.pppIpv6cpMsg.localIntfId, 
                        (sizeof(pEntry->Info.Ip6LocalIfID)-1));
            }

            if(strcmp(pppEventMsg.event.pppIpv6cpMsg.remoteIntfId , "") != 0)
            {
                memset(pEntry->Info.Ip6RemoteIfID,0,sizeof(pEntry->Info.Ip6RemoteIfID));
                strncpy(pEntry->Info.Ip6RemoteIfID,pppEventMsg.event.pppIpv6cpMsg.remoteIntfId, 
                        (sizeof(pEntry->Info.Ip6LocalIfID)-1));
            }

            /* set wan ipv6cp status */

            PppMgr_GenerateDuidFile(pEntry->Info.Name);

            PPPEventQData eventData = {0};

            eventData.action = PPPMGR_BUS_SET; 
            eventData.PppIfInstance = pEntry->Cfg.InstanceNumber; 
            eventData.comPath = WAN_COMPONENT_NAME;
            eventData.busPath = WAN_DBUS_PATH;
            eventData.keyPath = PPP_IPV6CP_STATUS_PARAM_NAME;
            strncpy(eventData.val, UP, sizeof(eventData.val) - 1);
            if (PppMgr_SendDataToQ(&eventData) != ANSC_STATUS_SUCCESS)
            {
                CcspTraceError(("%s %d - Failed to send data to Q\n", __FUNCTION__, __LINE__));
                PppMgr_GetIfaceData_release(pEntry);
                return ANSC_STATUS_FAILURE;
            }
            // After ipv6cp dbus set is called, allow some seconds for wan manager to set ipv6 config variables  
            // to avoid any race condition caused by next immediate dbus call for ipv4 status 
            // Race condition will cause dibbler client to restart multiple times
            // or causes stopping the dibbler client permanently due to misconfigured or overwritten ipv6 variables
            sleep(5);
        }
        else
        {
            PppMgr_RemoveDuidFile(pEntry->Info.Name);
            PppMgr_SetIPv6CPStatusDown(pEntry->Cfg.InstanceNumber);    
        }
        PppMgr_GetIfaceData_release(pEntry);
    }

    return ANSC_STATUS_SUCCESS;
}

/* --------------------------------------------------------------------
Function : ProcessIpcMsg

Decription: This API will process the message received from ppp client 
-----------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_ProcessPppState(ipc_msg_payload_t ipcMsg)
{

    ANSC_STATUS retStatus = ANSC_STATUS_SUCCESS;

    if( ipcMsg.msg_type != IPC_MSG_PPP_STATE_CHANGE )
    {
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d: PPP State change message from pid = %d\n", __FUNCTION__, __LINE__, ipcMsg.data.pppEventMsg.pid));

    int InstanceNumber = PppMgr_getIfaceDataWithPid(ipcMsg.data.pppEventMsg.pid);

    if(InstanceNumber == -1)
    {
        CcspTraceError(("%s %d: cannot find PPP Interface instance from pid : %d\n", __FUNCTION__, __LINE__, ipcMsg.data.pppEventMsg.pid));

        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d: handling incoming msg for PPP interface instance:%d\n", __FUNCTION__, __LINE__, InstanceNumber));

    switch(ipcMsg.data.pppEventMsg.pppState)
    {
        case    PPP_INTERFACE_UP:
        case    PPP_INTERFACE_DOWN:
        case    PPP_INTERFACE_UNCONFIGURED:
        case    PPP_INTERFACE_CONNECTING:
        case    PPP_INTERFACE_AUTHENTICATING:
        case    PPP_INTERFACE_PENDING_DISCONNET:
        case    PPP_INTERFACE_DISCONNECTING:
        case    PPP_INTERFACE_DISCONNECTED:
        case    PPP_INTERFACE_LCP_ECHO_FAILED:
        case    PPP_INTERFACE_AUTH_FAILED:
        case    PPP_LCP_AUTH_COMPLETED:
        case    PPP_LCP_VENDOR_RECEIVED:

            CcspTraceInfo(("[%s-%d] PPP_LCP_STATE_CHANGED message received\n", __FUNCTION__, __LINE__));

            if (PppMgr_ProcessStateChangedMsg(InstanceNumber, ipcMsg.data.pppEventMsg) == ANSC_STATUS_FAILURE)
            {
                CcspTraceError(("[%s-%d] Failed to proccess PPP_LCP_STATE_CHANGED  message \n",
                         __FUNCTION__, __LINE__));

                retStatus = ANSC_STATUS_FAILURE;
            } 
            break;
        case PPP_IPCP_COMPLETED:
        case PPP_IPCP_FAILED:

            CcspTraceInfo(("[%s-%d] PPP_NCP_IPCP_PARAM message received\n", __FUNCTION__, __LINE__));

            if(PppMgr_ProcessIpcpParams(InstanceNumber, ipcMsg.data.pppEventMsg) == ANSC_STATUS_FAILURE)
            {
                CcspTraceError(("[%s-%d] Failed to proccess PPP_NCP_IPCP_PARAM  message \n",
                         __FUNCTION__, __LINE__));

                retStatus = ANSC_STATUS_FAILURE;
            }
            break;
        case PPP_IPV6CP_COMPLETED:
        case PPP_IPV6CP_FAILED:

            CcspTraceInfo(("[%s-%d] PPP_NCP_IPCP6_PARAM message received\n", __FUNCTION__, __LINE__));

            if(PppMgr_ProcessIpv6cpParams(InstanceNumber, ipcMsg.data.pppEventMsg) == ANSC_STATUS_FAILURE)
            {
                CcspTraceError(("[%s-%d] Failed to proccess PPP_NCP_IPCP_PARAM  message \n",
                         __FUNCTION__, __LINE__));

                retStatus = ANSC_STATUS_FAILURE;
            }
            break;
        default:

            CcspTraceInfo(("[%s-%d] Unknow message type %d received", __FUNCTION__, __LINE__, ipcMsg.msg_type));

            retStatus = ANSC_STATUS_FAILURE;
            
            break;
    } 

    return retStatus;
}

static int PppMgr_ProcessPppEvent(PPPEventQData * pEventData) 
{
    if (pEventData == NULL || pEventData->PppIfInstance <= 0 || pEventData->val == NULL)
    {
        CcspTraceError(("%s %d: invalid args\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d: Reading from Queue - action:%s PppIfInstance:%d keyPath=%s val=%s\n",
                __FUNCTION__, __LINE__, pEventData->action == PPPMGR_BUS_SET?"PPPMGR_BUS_SET":"PPPMGR_EXEC_PPP_CLIENT", pEventData->PppIfInstance, pEventData->keyPath, pEventData->val));


    if (pEventData->action == PPPMGR_BUS_SET && pEventData->keyPath && pEventData->comPath && pEventData->busPath)
    {
        PDML_PPP_IF_FULL  pEntry = NULL;
        int WanInstanceNumber = -1;
        int WanVirtIfaceInstance = -1;
        
        char key[DATAMODEL_PARAM_LENGTH] = {0};
        
        pEntry = PppMgr_GetIfaceData_locked (pEventData->PppIfInstance);
        if (pEntry != NULL)
        {  
            WanInstanceNumber = pEntry->Cfg.WanInstanceNumber;
            WanVirtIfaceInstance = pEntry->Cfg.WanVirtIfaceInstance;
            PppMgr_GetIfaceData_release(pEntry);
        }


        snprintf(key, sizeof(key) - 1, pEventData->keyPath, WanInstanceNumber, WanVirtIfaceInstance);

        if(DmlWanmanagerSetParamValues(pEventData->comPath, pEventData->busPath, key,
                    pEventData->val, ccsp_string, TRUE ) == ANSC_STATUS_SUCCESS)
        {
            CcspTraceInfo(("Successfully set %s with value %s\n", key, pEventData->val));
            return ANSC_STATUS_SUCCESS;
        }
        CcspTraceError(("%s %d: failed to set %s = %s\n", __FUNCTION__, __LINE__, key, pEventData->val));
    }
    else if (pEventData->action == PPPMGR_EXEC_PPP_CLIENT)
    {
        UINT InstanceNumber = pEventData->PppIfInstance;
        PppMgr_StartPppClient(InstanceNumber);

        return ANSC_STATUS_SUCCESS;

    }
    return ANSC_STATUS_FAILURE;
}

/* --------------------------------------------------------------------
Function : PppMgr_EventHandlerThread

Decription: IPC thread function
-----------------------------------------------------------------------*/
static void* PppMgr_EventHandlerThread( void *arg )
{

    //detach thread from caller stack
    pthread_detach(pthread_self());

    // local variables
    BOOL bRunning = TRUE;

    ipc_msg_payload_t sockMsg;
    int msgSize = 0;; 

    mqd_t     mq;
    struct    mq_attr attr;
    /* initialize the queue attributes */
    attr.mq_flags   = 0;
    attr.mq_maxmsg  = MAX_QUEUE_LENGTH;
    attr.mq_msgsize = sizeof(PPPEventQData);
    attr.mq_curmsgs = 0;

    PPPEventQData    eventMsg = { 0 };

    /* create the message queue */
    mq = mq_open(PPP_EVENT_QUEUE_NAME, O_CREAT | O_RDONLY | O_NONBLOCK, 0644, &attr);

    if (-1 == mq)
    {
        CcspTraceError(("%s %d:Unable to open message queue: %s\n", __FUNCTION__, __LINE__, strerror(errno)));
        PppMgr_closeIpcSocket(ipcListenFd);
        pthread_exit(NULL);
        return NULL;
    }


    while (bRunning)
    {
        msgSize = 0;
        memset (&sockMsg, 0, sizeof(ipc_msg_payload_t));
        memset (&eventMsg, 0, sizeof(PPPEventQData));

        PppMgr_receiveIpcSocket(ipcListenFd, (char*)&sockMsg, (uint32_t *)&msgSize);

        if(msgSize > 0)
        {
            CcspTraceInfo(("[%s-%d] Message received  with size %d\n", __FUNCTION__, __LINE__, msgSize));
            PppMgr_ProcessPppState(sockMsg);
        }

        /* receive the message */
        msgSize = mq_receive(mq, (char *)&eventMsg, sizeof(PPPEventQData), NULL);
        if (msgSize > 0)
        {
            CcspTraceInfo(("%s %d: Q not empty\n", __FUNCTION__, __LINE__));
            PppMgr_ProcessPppEvent(&eventMsg);
        }
        usleep(500000);
    }

    mq_close(mq);
    PppMgr_closeIpcSocket(ipcListenFd);

    pthread_exit(NULL);
}

/* --------------------------------------------------------------------
Function : PppMgr_IpcServerInit

Decription: This API will create and bind the IPC socket
-----------------------------------------------------------------------*/
static ANSC_STATUS PppMgr_IpcServerInit()
{
    //create and bind the socket if this is server
    if(ANSC_STATUS_FAILURE == PppMgr_createIpcSocket(&ipcListenFd, PPP_MGR_IPC_SERVER))
    {
        return ANSC_STATUS_FAILURE;

    }

    return ANSC_STATUS_SUCCESS;
}

/* DmlPppSyseventInit() */
static int DmlPppSyseventInit( void )
{
    char sysevent_ip[] = "127.0.0.1";
    char sysevent_name[] = "pppmgr";

    sysevent_fd =  sysevent_open( sysevent_ip, SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, sysevent_name, &sysevent_token );

    if ( sysevent_fd < 0 ){
        return -1;
    }

    return 0;
}

ANSC_STATUS PppMgr_Init()
{
    /* Start IPC server to receive events from ppp client */
    if( PppMgr_StartIpcServer() == ANSC_STATUS_FAILURE )
    {
        CcspTraceInfo(("%s %d - IPC server start failed!\n", __FUNCTION__, __LINE__ ));

        return ANSC_STATUS_FAILURE;
    }
    CcspTraceInfo(("%s %d - IPC server started successfully!\n", __FUNCTION__, __LINE__ ));

    // Initialise syscfg
    if (syscfg_init() < 0)
    {
        CcspTraceError(("failed to initialise syscfg"));
        return ANSC_STATUS_FAILURE;
    }

    // Initialize sysevent
    if ( DmlPppSyseventInit( ) < 0 )
    {
        return ANSC_STATUS_FAILURE;
    }

    return ANSC_STATUS_SUCCESS;

}

