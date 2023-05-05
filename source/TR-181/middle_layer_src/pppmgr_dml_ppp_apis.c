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

#include "pppmgr_ssp_global.h"
#include "pppmgr_dml_plugin_main_apis.h"
#include "pppmgr_dml_ppp_apis.h"
#include "pppmgr_dml.h"
#include <regex.h>
#include <dirent.h>

#define NET_STATS_FILE "/proc/net/dev"
#define PPPoE_VLAN_IF_NAME  "vlan101"
#define GET_PPPID_ATTEMPT 5
#define PPP_LCPEcho 30
#define PPP_LCPEchoRetry 3

static int CosaUtilGetIfStats(char *ifname, PDML_IF_STATS pStats);
extern char g_Subsystem[32];
extern ANSC_HANDLE bus_handle;

extern PBACKEND_MANAGER_OBJECT               g_pBEManager;
static void* PppMgr_StartPppdDaemon( void *arg );
static void* PppMgr_ResetPppdDaemon( void *arg);

ANSC_STATUS
PppDmlGetSupportedNCPs
    (
        ANSC_HANDLE                 hContext,
        PULONG                      puLong
    )
{
    *puLong = (DML_PPP_SUPPORTED_NCP_IPCP | DML_PPP_SUPPORTED_NCP_IPv6CP);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS GetNumOfInstance (int *count)
{

    int ret_val = ANSC_STATUS_SUCCESS;
    int retPsmGet = CCSP_SUCCESS;
    char* param_value = NULL;

    retPsmGet = PSM_Get_Record_Value2(bus_handle,g_Subsystem, PSM_PPPMANAGER_PPPIFCOUNT, NULL, &param_value);
    if (retPsmGet != CCSP_SUCCESS) { \
        AnscTraceFlow(("%s Error %d reading %s %s\n", __FUNCTION__, retPsmGet, PSM_PPPMANAGER_PPPIFCOUNT, param_value));
        ret_val = ANSC_STATUS_FAILURE;
    }
    else if(param_value != NULL) {
        sscanf(param_value, "%d", count);
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(param_value);
    }

    return ret_val;

}


ULONG DmlGetTotalNoOfPPPInterfaces
(
  ANSC_HANDLE                 hContext
)
{
    int ppp_if_count = 0 ;

    GetNumOfInstance(&ppp_if_count);

    return ppp_if_count;
}


ANSC_STATUS
PppDmlGetIfStats
    (
        ANSC_HANDLE                 hContext,
        ULONG                       ulPppIfInstanceNumber,
        PDML_IF_STATS               pStats,
        PDML_PPP_IF_FULL            pEntry
)
{
    char wan_interface[10] = {0};

    AnscCopyString( wan_interface, pEntry->Info.Name);
    CosaUtilGetIfStats(wan_interface,pStats);
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
PPPDmlGetIfInfo
    (
        ANSC_HANDLE                 hContext,
        ULONG                       ulInstanceNumber,
        PDML_PPP_IF_INFO       pInfo
    )
{
    if ( pInfo == NULL )
    {
        return ANSC_STATUS_FAILURE;
    }

    //TODO Need to revisit
    /*not supported*/
    pInfo->EncryptionProtocol  =  DML_PPP_ENCRYPTION_None;
    pInfo->CompressionProtocol = DML_PPP_COMPRESSION_None;

    /*hardcoded by backend*/
    pInfo->LCPEchoRetry = PPP_LCPEchoRetry;
    pInfo->LCPEcho      = PPP_LCPEcho;

    pInfo->SessionID    = 0;
    get_session_id(&pInfo->SessionID, hContext);

    return ANSC_STATUS_SUCCESS;
}

BOOL
PppDmlIfEnable
    (
        ANSC_HANDLE                 hContext,
        ULONG                       ulInstanceNumber,
        PDML_PPP_IF_FULL            pEntry
    )
{

    char command[1024] = { 0 };
    char config_command[1024] = { 0 };
    char service_name[256] = { 0 };
    char auth_proto[8] = { 0 };
    pthread_t pppdThreadId;
    char buff[64] = { 0 };
    char vlan_id[10] = { 0 };
    char physical_interface[14]= { 0 };
    char VLANInterfaceName[32] = { 0 };
    int ret ;
    char acTmpQueryParam[256] = {0};

    pthread_mutex_lock(&pEntry->mDataMutex);

    if(pEntry->Cfg.bEnabled == true )
    {
        CcspTraceInfo (("%s %d: enabling PPP on interface %d\n", __FUNCTION__, __LINE__, ulInstanceNumber));
        if(ANSC_STATUS_SUCCESS == PppMgr_checkPidExist(pEntry->Info.pppPid))
        {
            CcspTraceInfo(("pppd is already running \n"));
            pthread_mutex_unlock(&pEntry->mDataMutex);
            return TRUE;
        }

        pEntry->Info.pppPid = 0;

        // get UserName & Password from hal
        platform_hal_GetPppUserName (pEntry->Cfg.Username, sizeof(pEntry->Cfg.Username));
        platform_hal_GetPppPassword (pEntry->Cfg.Password, sizeof(pEntry->Cfg.Password));

        if((strcmp(pEntry->Info.Name,"") != 0) && (strcmp(pEntry->Info.InterfaceServiceName,"") != 0) &&
                (strcmp(pEntry->Cfg.Username,"") != 0) && (strcmp(pEntry->Cfg.Password,"") != 0) &&
                (pEntry->Info.AuthenticationProtocol > 0))
        {
            if((pEntry->Info.AuthenticationProtocol == DML_PPP_AUTH_CHAP) ||
                    (pEntry->Info.AuthenticationProtocol ==  DML_PPP_AUTH_PAP))
            {
                sprintf(auth_proto,"0");
            }
            else
            {
                /* support for mschap */
                sprintf(auth_proto,"4");
            }
            if(pEntry->Cfg.LinkType == DML_PPPoA_LINK_TYPE)
            {
                CcspTraceInfo(("%s %d: PPPoA_LINK_TYPE: constructing arguments\n", __FUNCTION__, __LINE__));
#ifdef USE_PPP_DAEMON
                snprintf(command, sizeof(command), "pppd -6 -c %s -a %s -u %s -p %s -f %s &",
                        pEntry->Info.Name, pEntry->Info.InterfaceServiceName, pEntry->Cfg.Username,
                        pEntry->Cfg.Password, auth_proto);
#else
                /* Assume a default rp-pppoe config exist. Update rp-pppoe configuration */
                ret =  snprintf(config_command, sizeof(config_command), "pppoe_config.sh %s %s %s %s PPPoA %d %d",
                        pEntry->Cfg.Username, pEntry->Cfg.Password, pEntry->Info.InterfaceServiceName, pEntry->Info.Name, pEntry->Info.LCPEcho, pEntry->Info.LCPEchoRetry);
                if(ret > 0 && ret <= sizeof(config_command))
                {
                    system(config_command);
                }
                /* start rp-pppoe */
                ret = snprintf(command, sizeof(command), "/usr/sbin/pppoe-start");
                if(ret > 0 && ret <= sizeof(command))
                {
                    CcspTraceInfo((" successfully started rp-pppoe \n"));
                }
#endif
            }
            else if (pEntry->Cfg.LinkType == DML_PPPoE_LINK_TYPE)
            {
                CcspTraceInfo(("%s %d: PPPoE_LINK_TYPE: constructing arguments\n", __FUNCTION__, __LINE__));
#ifdef USE_PPP_DAEMON
                snprintf(command, sizeof(command), "pppd -6 -c %s -i %s -u %s -p %s -f %s &",
                        pEntry->Info.Name, PPPoE_VLAN_IF_NAME, pEntry->Cfg.Username, pEntry->Cfg.Password, auth_proto);
#else
                ret = snprintf(acTmpQueryParam, sizeof(acTmpQueryParam),"%s.%s",pEntry->Cfg.LowerLayers,"Name");
                if(ret > 0 && ret <= sizeof(config_command))
                {
                    if(ANSC_STATUS_FAILURE == DmlPppMgrGetParamValues(WAN_COMPONENT_NAME, WAN_DBUS_PATH, acTmpQueryParam, VLANInterfaceName))
                    {
                        CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
                        pthread_mutex_unlock(&pEntry->mDataMutex);
                        return ANSC_STATUS_FAILURE;
                    } 
                }

                /* Assume a defule rp-pppoe config exist. Update rp-pppoe configuration */
                ret = snprintf(config_command, sizeof(config_command), "pppoe_config.sh '%s' '%s' %s %s PPPoE %d %d %d ",
                        pEntry->Cfg.Username, pEntry->Cfg.Password, PPPoE_VLAN_IF_NAME, pEntry->Info.Name, pEntry->Info.LCPEcho , pEntry->Info.LCPEchoRetry,pEntry->Cfg.MaxMRUSize);
                if(ret > 0 && ret <= sizeof(config_command))
                {
                    system(config_command);
                }

                /* start rp-pppoe */
                ret = snprintf(command, sizeof(command), "/usr/sbin/pppoe-start");
                if(ret > 0 && ret <= sizeof(command))
                {
                    CcspTraceInfo((" successfully started rp-pppoe \n"));
                }
#endif
            }
            CcspTraceInfo(("parameters were set\n"));
        }
        else
        {
            CcspTraceError(("%s %d: unable to get PPP params\n", __FUNCTION__, __LINE__));
            pthread_mutex_unlock(&pEntry->mDataMutex);
            return FALSE;
        }
        CcspTraceInfo(("command to execute is  '%s'\n", command));
        PPPEventQData eventData = {0};

        eventData.action = PPPMGR_EXEC_PPP_CLIENT;
        eventData.WANInstance = ulInstanceNumber;
        strncpy(eventData.val, command, sizeof(eventData.val) - 1);

        if (PppMgr_SendDataToQ(&eventData) != ANSC_STATUS_SUCCESS)
        {
            CcspTraceError(("%s %d - Failed to send data to Q\n", __FUNCTION__, __LINE__));
            pthread_mutex_unlock(&pEntry->mDataMutex);
            return FALSE;
        }

        // TODO - check lock in this function
        /* lock while updating pid */

        // update Wan Interface instance 
        DmlPppMgrGetWanMgrInstanceNumber(pEntry->Cfg.LowerLayers, &(pEntry->Cfg.WanInstanceNumber));
        CcspTraceInfo(("%s %d: PPP Interface %d WanInterface instance: %d\n", __FUNCTION__, __LINE__, pEntry->Cfg.InstanceNumber, pEntry->Cfg.WanInstanceNumber));

    }
    else
    {
        CcspTraceInfo (("%s %d: disabling PPP on interface %d\n", __FUNCTION__, __LINE__, ulInstanceNumber));
#ifdef USE_PPP_DAEMON
        PppMgr_stopPppProcess(pEntry->Info.pppPid);

        pEntry->Info.pppPid = 0;
#else
        PppMgr_stopPppoe();
#endif
    }

    pthread_mutex_unlock(&pEntry->mDataMutex);
    return TRUE;
}

ANSC_STATUS
PppDmlIfReset
    (
        ANSC_HANDLE                 hContext,
        ULONG                       ulInstanceNumber,
        PDML_PPP_IF_FULL            pEntry
    )
{
    pthread_t pppdThreadId;
    PRESET_THREAD_ARGS pthread_args;
    pthread_args = malloc(sizeof(RESET_THREAD_ARGS) * 1);
    pthread_args->pEntry=pEntry;
    pthread_args->ulInstanceNumber = ulInstanceNumber;
    int iErrorCode = pthread_create( &pppdThreadId, NULL, &PppMgr_ResetPppdDaemon,pthread_args);
    if( 0 != iErrorCode )
    {
        CcspTraceInfo(("%s %d - Failed to start Pppmgr_ResetPppdDaemon  %d\n", __FUNCTION__, __LINE__,iErrorCode ));
    }

    return ANSC_STATUS_SUCCESS;
}

static void* PppMgr_ResetPppdDaemon( void *arg )
{
    //TODO : Need to Revisit
    PRESET_THREAD_ARGS pReset = arg;
    pReset->pEntry->Cfg.bEnabled = false;
    sleep(5);
    PppDmlIfEnable(NULL,pReset->ulInstanceNumber,pReset->pEntry);
    sleep(10);
    pReset->pEntry->Cfg.bEnabled = true;
    PppDmlIfEnable(NULL,pReset->ulInstanceNumber,pReset->pEntry);
    free(arg);
    pthread_exit(NULL);
    return NULL;
}

ANSC_STATUS
PppDmlGetIfCfg
    (
        ANSC_HANDLE                 hContext,
        PDML_PPP_IF_CFG        pCfg        /* Identified by InstanceNumber */
    )
{
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
PppDmlAddIfEntry
    (
        ANSC_HANDLE                 hContext,
        PDML_PPP_IF_FULL       pEntry
    )
{
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
PppDmlDelIfEntry
    (
        ANSC_HANDLE                 hContext,
        ULONG                       ulInstanceNumber
    )
{
    return ANSC_STATUS_SUCCESS;
}

int validateUsername( char* pString)
{
    int ret=-1;
    regex_t reg;
    const char *Url_Pattern= "^[a-zA-Z0-9_#-.@]*$";
    regcomp(&reg ,Url_Pattern, REG_EXTENDED);
    ret = regexec(&reg, pString, 0, NULL, 0);
    if (ret == 0) {
        return 0;
    }
    else {
       CcspTraceWarning(("Invalid username '%s'\n", pString));
       return -1;
    }
}

int PppMgr_RdkBus_SetParamValuesToDB( char *pParamName, char *pParamVal )
{
    int     retPsmSet  = CCSP_SUCCESS;
    /* Input Validation */
    if( ( NULL == pParamName) || ( NULL == pParamVal ) )
    {
        CcspTraceError(("%s Invalid Input Parameters\n",__FUNCTION__));
        return CCSP_FAILURE;
    }

    retPsmSet = PSM_Set_Record_Value2(bus_handle,g_Subsystem, pParamName, ccsp_string, pParamVal);
    if (retPsmSet != CCSP_SUCCESS) {
        CcspTraceError(("%s Error %d writing %s\n", __FUNCTION__, retPsmSet, pParamName));
    }

    return retPsmSet;
}

static int PppManager_SetParamFromPSM(PDML_PPP_IF_FULL pEntry)
{
    int retPsmSet = CCSP_SUCCESS;
    char param_name[256] = {0};
    char param_value[256] = {0};
    int instancenum = 0;

    instancenum = pEntry->Cfg.InstanceNumber;

    CcspTraceWarning(("%s-%d:instancenum=%d \n",__FUNCTION__, __LINE__, instancenum));

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));

    sprintf(param_value, "%d", pEntry->Cfg.IdleDisconnectTime);
    sprintf(param_name, PSM_PPP_IDLETIME, instancenum);
    PppMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));
	
    sprintf(param_value, "%d", pEntry->Cfg.MaxMRUSize);
    sprintf(param_name, PSM_PPP_MAXMRUSIZE, instancenum);
    PppMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
PppDmlSetIfCfg
    (
        ANSC_HANDLE                 hContext,
        PDML_PPP_IF_CFG        pCfg        /* Identified by InstanceNumber */
    )
{
     int ret_val = ANSC_STATUS_SUCCESS;
     ret_val = PppManager_SetParamFromPSM(pCfg);
     if(ret_val != ANSC_STATUS_SUCCESS)
     {
         CcspTraceError(("%s %d Failed \n", __FUNCTION__, __LINE__));
     }

     return ret_val;
}

ANSC_STATUS
PppDmlSetIfValues
    (
        ANSC_HANDLE                 hContext,
        ULONG                       ulIndex,
        ULONG                       ulInstanceNumber,
        char*                       pAlias
    )
{
    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
PppDmlGetIfEntry
    (
        ANSC_HANDLE                 hContext,
        ULONG                       ulIndex,
        PDML_PPP_IF_FULL       pEntry
    )
{
    if (!pEntry)
    {
        return ANSC_STATUS_FAILURE;
    }

    PppDmlGetIntfValuesFromPSM(hContext,ulIndex+1,pEntry);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
PppDmlGetIntfValuesFromPSM
                (
        ANSC_HANDLE hContext,
        ULONG  ulIndex,
        PDML_PPP_IF_FULL pEntry
      )
{
    int retPsmGet = CCSP_SUCCESS;
    char* param_value = NULL;
    char param_name[256]= {0};
    char buff[10];

    if (!pEntry)
        {
        return ANSC_STATUS_FAILURE;
        }

    // TODO : init mutex
    pthread_mutexattr_t     muttex_attr;
    pthread_mutexattr_settype(&muttex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(pEntry->mDataMutex), &(muttex_attr));

    /* Get Alias */
    sprintf(param_name, PSM_PPP_IF_ALIAS, ulIndex);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL)
    {
        sscanf(param_value, "%s", pEntry->Cfg.Alias);
        CcspTraceInfo(("%s %d: from PSM %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value));
    }
    else
    {
        CcspTraceError(("%s %d: failed to get %s from PSM\n", __FUNCTION__, __LINE__, param_name));
    }

    /* Get service name */
    sprintf(param_name, PSM_PPP_IF_SERVICE_NAME,ulIndex);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL)
    {
        sscanf(param_value, "%s", pEntry->Info.InterfaceServiceName);
        CcspTraceInfo(("%s %d: from PSM %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value));
    }
    else
    {
        CcspTraceError(("%s %d: failed to get %s from PSM\n", __FUNCTION__, __LINE__, param_name));
    }

    /* Get interface name */
    sprintf(param_name, PSM_PPP_IF_NAME,ulIndex);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL)
        {
        sscanf(param_value, "%s", pEntry->Info.Name);
        CcspTraceInfo(("%s %d: from PSM %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value));
    }
    else
    {
        CcspTraceError(("%s %d: failed to get %s from PSM\n", __FUNCTION__, __LINE__, param_name));
        }

    /* Get authentication protocol */
    sprintf(param_name, PSM_PPP_AUTH_PROTOCOL,ulIndex);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL)
    {
        if (strcmp(param_value, "CHAP") == 0)
        {
            pEntry->Info.AuthenticationProtocol = DML_PPP_AUTH_CHAP;
        }
        CcspTraceInfo(("%s %d: from PSM %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value));
    }
    else
    {
        CcspTraceError(("%s %d: failed to get %s from PSM\n", __FUNCTION__, __LINE__, param_name));
    }

    /* Get last connection error */
    sprintf(param_name, PSM_PPP_LAST_COONECTION_ERROR,ulIndex);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL)
    {
        if(strcmp(param_value,"DML_PPP_CONN_ERROR_NONE") == 0)
            {
            pEntry->Info.LastConnectionError = DML_PPP_CONN_ERROR_NONE;
        }
        CcspTraceInfo(("%s %d: from PSM %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value));
            }
    else
    {
        CcspTraceError(("%s %d: failed to get %s from PSM\n", __FUNCTION__, __LINE__, param_name));
        }

    /* Get idle time  */
    sprintf(param_name, PSM_PPP_IDLETIME, ulIndex);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL)
        {
        sscanf(param_value, "%s",buff);
        pEntry->Cfg.IdleDisconnectTime = atoi(buff);
        CcspTraceInfo(("%s %d: from PSM %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value));
    }
    else
    {
        CcspTraceError(("%s %d: failed to get %s from PSM\n", __FUNCTION__, __LINE__, param_name));
    }

    /* Get max mru size  */
    sprintf(param_name, PSM_PPP_MAXMRUSIZE, ulIndex);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL)
            {
        sscanf(param_value, "%s",buff);
        pEntry->Cfg.MaxMRUSize = atoi(buff);
        CcspTraceInfo(("%s %d: from PSM %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value));
            }
    else
    {
        CcspTraceError(("%s %d: failed to get %s from PSM\n", __FUNCTION__, __LINE__, param_name));
        }

    /* Get link type */
    sprintf(param_name,PSM_PPP_LINK_TYPE,ulIndex);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL)
    {
        if(strcmp(param_value,"PPPoA") == 0)
        {
            pEntry->Cfg.LinkType = DML_PPPoA_LINK_TYPE;
        }
        else if(strcmp(param_value,"PPPoE") == 0)
        {
            pEntry->Cfg.LinkType = DML_PPPoE_LINK_TYPE;
        }

        CcspTraceInfo(("%s %d: from PSM %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value));
        }
    else
        {
        CcspTraceError(("%s %d: failed to get %s from PSM\n", __FUNCTION__, __LINE__, param_name));
        }

    /* Get LowerLayer */
    sprintf(param_name, PSM_PPP_LOWERLAYERS, ulIndex);
    retPsmGet = PSM_Get_Record_Value2(bus_handle, g_Subsystem, param_name, NULL, &param_value);
    if (retPsmGet == CCSP_SUCCESS && param_value != NULL)
        {
        strncpy(pEntry->Cfg.LowerLayers, param_value, sizeof(pEntry->Cfg.LowerLayers) - 1);
        CcspTraceInfo(("%s %d: from PSM %s = %s\n", __FUNCTION__, __LINE__, param_name, param_value));
        }
    else
    {
        CcspTraceError(("%s %d: failed to get %s from PSM\n", __FUNCTION__, __LINE__, param_name));
    }


    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS PppMgr_checkPidExist(pid_t pppPid)
{

    pid_t pid = 0;
    char line[64] = { 0 };
    FILE *command = NULL;

    if(pppPid)
    {
        command = popen("ps | grep pppd | grep -v grep | awk '{print $1}'","r");

        if(command != NULL)
    {
            while(fgets(line, 64, command))
        {
                pid = strtoul(line, NULL,10);

                if(pid == pppPid)
            {
                    pclose(command);
                return ANSC_STATUS_SUCCESS;
            }
        }
            pclose(command);
        }

    }
    return ANSC_STATUS_FAILURE;
}

ANSC_STATUS PppMgr_stopPppProcess(pid_t pid)
{

    kill(pid, SIGKILL);

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS PppMgr_stopPppoe(void)
{
    system("/usr/sbin/pppoe-stop");

    return ANSC_STATUS_SUCCESS;
}

/*
 * find_strstr ()
 * @description: /proc/pid/cmdline contains command line args in format "args1\0args2".
                 This function will find substring even if there is a end of string character
 * @params     : basestr - base string eg: "hello\0world"
                 basestr_len - length of basestr eg: 11 for "hello\0world"
                 substr - sub string eg: "world"
                 substr_len - length of substr eg: 5 for "world"
 * @return     : SUCCESS if matches, else returns failure
 *
 */
int find_strstr (char * basestr, int basestr_len, char * substr, int substr_len)
{
    if ((basestr == NULL) || (substr == NULL))
    {
        return ANSC_STATUS_FAILURE;
    }

    if (basestr_len <= substr_len)
    {
        return ANSC_STATUS_FAILURE;
    }

    int i = 0, j = 0;

    for (i = 0; i < basestr_len; i++)
    {
        if (basestr[i] == substr[j])
    {
            for (; ((j < substr_len) && (i < basestr_len)); j ++, i++)
            {
                if (basestr[i] != substr[j])
        {
                    j=0;
                    break;
                }

                if (j == substr_len - 1)
                    return ANSC_STATUS_SUCCESS;
            }
        }
    }
    return ANSC_STATUS_FAILURE;
}

/*
 * strtol64 ()
 * @description: utility call to check if string is a decimal number - used to check if /proc/<pid> is actually as pid
 * @params     : str - string to check if its a number
                 endptr - output param to point to end of string
                 val - output param to send out the pid
 * @return     : if str is a pid then SUCCESS, else FAILURE
 *
 */
static int strtol64(const char *str, char **endptr, int32_t base, int64_t *val)
{
    int ret = SUCCESS;
    char *localEndPtr=NULL;

    errno = 0;  /* set to 0 so we can detect ERANGE */

    *val = strtoll(str, &localEndPtr, base);

    if ((errno != 0) || (*localEndPtr != '\0'))
    {
        *val = 0;
        ret = ANSC_STATUS_FAILURE;
    }

    if (endptr != NULL)
    {
        *endptr = localEndPtr;
    }

    return ret;
}


/*
 * check_proc_entry_for_pid ()
 * @description: check the contents of /proc directory to match the process name
 * @params     : name - process name
                 args - optional parameter - can check running process argument and return
                 eg: if 2 pppd are running 1) pppd -i erouter0 2) pppd -i erouter1
                 if args == "-ierouter0", pid of first udhcpc is returned
 * @return     : returns the pid if proc entry exists
 *
 */
static int check_proc_entry_for_pid (char * name, char * args)
{
    if (name == NULL)
    {
        CcspTraceError(("%s %d: Invalid args\n", __FUNCTION__, __LINE__));
        return 0;
    }

    DIR *dir;
    FILE *fp;
    struct dirent *dent;
    bool found=false;
    int rc, p, i;
    int64_t pid;
    int rval = 0;
    char processName[256];
    char cmdline[512] = {0};
    char filename[256];
    char status = 0;

    if (NULL == (dir = opendir("/proc")))
    {
        CcspTraceError(("%s %d:could not open /proc\n", __FUNCTION__, __LINE__));
        return 0;
    }

    while (!found && (dent = readdir(dir)) != NULL)
    {
        if ((dent->d_type == DT_DIR) &&
                (ANSC_STATUS_SUCCESS == strtol64(dent->d_name, NULL, 10, &pid)))
        {
            snprintf(filename, sizeof(filename), "/proc/%lld/stat", (long long int) pid);
            fp = fopen(filename, "r");
            if (fp == NULL)
            {
                continue;
            }
            memset(processName, 0, sizeof(processName));
            rc = fscanf(fp, "%d (%255s %c ", &p, processName, &status);
            fclose(fp);

            if (rc >= 2)
            {
                i = strlen(processName);
                if (i > 0)
                {
                    if (processName[i-1] == ')')
                        processName[i-1] = 0;
                }
            }

            if (!strcmp(processName, name))
            {
                if ((status == 'R') || (status == 'S'))
                {
                    if (args != NULL)
                    {
                        // argument to be verified before returning pid
                        CcspTraceInfo(("%s %d: %s running in pid %lld.. checking for cmdline param %s\n", __FUNCTION__, __LINE__, name, (long long int) pid, args));
                        snprintf(filename, sizeof(filename), "/proc/%lld/cmdline", (long long int) pid);
                        fp = fopen(filename, "r");
                        if (fp == NULL)
                        {
                            CcspTraceError(("%s %d: could not open %s\n", __FUNCTION__, __LINE__, filename));
                            continue;
                        }
                        CcspTraceInfo(("%s %d: opening file %s\n", __FUNCTION__, __LINE__, filename));

                        memset (cmdline, 0, sizeof(cmdline));
                        int num_read ;
                        if ((num_read = fread(cmdline, 1, sizeof(cmdline)-1 , fp)) > 0)
                        {
                            cmdline[num_read] = '\0';
                            CcspTraceInfo(("%s %d: comparing cmdline from proc:%s with %s\n", __FUNCTION__, __LINE__, cmdline, args));
                            if (find_strstr(cmdline, sizeof(cmdline), args, strlen(args)) == ANSC_STATUS_SUCCESS)
                            {
                                rval = pid;
                                found = true;
                            }
                        }

                        fclose(fp);
                    }
                    else
                    {
                        // no argument passed, so return pid of running process
                        rval = pid;
                        found = true;
                    }
                }
                else
                {
                    CcspTraceError(("%s %d: %s running, but is in %c mode\n", __FUNCTION__, __LINE__, filename, status));
                }
            }
        }
    }

    closedir(dir);

    return rval;

}

pid_t PppMgr_getPppPid(char * ifname)
{
    int waitTime = 0;
    pid_t pid = 0;

    while (waitTime <= GET_PPPID_ATTEMPT)
    {
        pid = check_proc_entry_for_pid("pppd", ifname);

        if (pid != 0)
                {
            break;
        }
        sleep(1);
        waitTime++;
    }

#if 0
    char line[64] = { 0 };
    FILE *command = NULL;
    pid_t pid = 0;

    command = popen("pidof pppd", "r");

    if(command != NULL)
    {
        fgets(line, 64, command);

        pid = strtoul(line, NULL,10);

        pclose(command);
    }
    return pid;
#endif
}

static ANSC_STATUS DmlPppMgrGetParamValues(char *pComponent, char *pBus, char *pParamName, char *pReturnVal)
{
    CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    parameterValStruct_t **retVal;
    char *ParamName[1];
    int ret = 0,
        nval;

    //Assign address for get parameter name
    ParamName[0] = pParamName;

    ret = CcspBaseIf_getParameterValues(
        bus_handle,
        pComponent,
        pBus,
        ParamName,
        1,
        &nval,
        &retVal);

    //Copy the value
    if (CCSP_SUCCESS == ret)
    {
        CcspTraceWarning(("%s parameterValue[%s]\n", __FUNCTION__, retVal[0]->parameterValue));

        if (NULL != retVal[0]->parameterValue)
        {
            memcpy(pReturnVal, retVal[0]->parameterValue, strlen(retVal[0]->parameterValue) + 1);
        }

        if (retVal)
        {
            free_parameterValStruct_t(bus_handle, nval, retVal);
        }

        return ANSC_STATUS_SUCCESS;
    }

    if (retVal)
    {
        free_parameterValStruct_t(bus_handle, nval, retVal);
    }

    return ANSC_STATUS_FAILURE;
}

ANSC_STATUS DmlPppMgrGetWanMgrInstanceNumber(char *pLowerLayers, INT *piInstanceNumber)
{
    char acTmpReturnValue[256] = {0};
    char acTmpQueryParam[256] = {0};
    INT iLoopCount;
    INT iTotalNoofEntries;

    //Validate buffer
    if ((NULL == pLowerLayers) || (NULL == piInstanceNumber))
    {
        CcspTraceError(("%s Invalid Buffer\n", __FUNCTION__));
        return ANSC_STATUS_FAILURE;
    }

    //Initialise default value
    *piInstanceNumber = -1;

    if (ANSC_STATUS_FAILURE == DmlPppMgrGetParamValues(WAN_COMPONENT_NAME, WAN_DBUS_PATH, WAN_NOE_PARAM_NAME, acTmpReturnValue))
    {
    CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
    return ANSC_STATUS_FAILURE;
    }

    //Total count
    iTotalNoofEntries = atoi(acTmpReturnValue);
    CcspTraceInfo(("%s %d - TotalNoofEntries:%d\n", __FUNCTION__, __LINE__, iTotalNoofEntries));

    if (0 >= iTotalNoofEntries)
    {
        CcspTraceError(("%s %d: Cannot get number of WanInterface Entry\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    //Traverse from loop
    for (iLoopCount = 0; iLoopCount < iTotalNoofEntries; iLoopCount++)
    {
        //Query
        snprintf(acTmpQueryParam, sizeof(acTmpQueryParam), WAN_PHY_PATH_PARAM_NAME, iLoopCount + 1);

        memset(acTmpReturnValue, 0, sizeof(acTmpReturnValue));
        if (ANSC_STATUS_FAILURE == DmlPppMgrGetParamValues(WAN_COMPONENT_NAME, WAN_DBUS_PATH, acTmpQueryParam, acTmpReturnValue))
        {
            CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
            continue;
        }

        //Compare name
        if (0 == strcmp(acTmpReturnValue, pLowerLayers))
        {
            *piInstanceNumber = iLoopCount + 1;
            break;
        }
    }

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS DmlWanmanagerSetParamValues(const char *pComponent, const char *pBus,
        const char *pParamName, const char *pParamVal, enum dataType_e type, unsigned int bCommitFlag)
{
    CCSP_MESSAGE_BUS_INFO *bus_info = (CCSP_MESSAGE_BUS_INFO *)bus_handle;
    parameterValStruct_t param_val[1] = {0};
    char *faultParam = NULL;
    int ret = 0;

    param_val[0].parameterName = pParamName;
    param_val[0].parameterValue = pParamVal;
    param_val[0].type = type;

    ret = CcspBaseIf_setParameterValues(
            bus_handle,
            pComponent,
            pBus,
            0,
            0,
            &param_val,
            1,
            bCommitFlag,
            &faultParam);


    if ((ret != CCSP_SUCCESS) && (faultParam != NULL))
    {
        CcspTraceError(("%s-%d Failed to set %s to %s ret = %d\n", __FUNCTION__, __LINE__, pParamName, pParamVal, ret));
        bus_info->freefunc(faultParam);
        return ANSC_STATUS_FAILURE;
    }
    CcspTraceInfo(("%s %d: successfully set %s to %s\n", __FUNCTION__, __LINE__, pParamName, pParamVal));

    return ANSC_STATUS_SUCCESS;
}

ULONG GetUptimeinSeconds ()
{
    char acGetParamValue[DATAMODEL_PARAM_LENGTH] = { 0 };
    ULONG UpTime = 0;

    if(DmlPppMgrGetParamValues(PANDM_COMPONENT_NAME, PANDM_DBUS_PATH, UP_TIME_PARAM_NAME, acGetParamValue) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d Failed to get UpTime value\n", __FUNCTION__, __LINE__));
    }
    else
    {
        sscanf(acGetParamValue, "%lu", &UpTime);
    }

    return UpTime;
}

#define PPPOE_PROC_FILE "/proc/net/pppoe"
static int get_session_id_from_proc_entry(ULONG * p_id)
{
    FILE * fp;
    char buf[1024] = {0};
    if(fp = fopen(PPPOE_PROC_FILE, "r"))
    {
        /* Skip first line of /proc/net/pppoe */
        /* Id Address Device */
        fgets(buf, sizeof(buf)-1, fp);
        while(fgets(buf, sizeof(buf)-1, fp))
        {
            unsigned long id = 0L;
            if(sscanf(buf, "%08X", &id) == 1)
            {
                *p_id = ntohs(id);
                CcspTraceInfo(("PPP Session ID: %08X, %d \n", id, *p_id));
            }
        }
        fclose(fp);
    }
    return 0;
}

static int CosaUtilGetIfStats(char *ifname, PDML_IF_STATS pStats)
{
    int    i;
    FILE *fp;
    char buf[1024] = {0} ;
    char *p;
    int    ret = 0;

    if (ifname == NULL)
        return -1;

    fp = fopen(NET_STATS_FILE, "r");

    if(fp)
    {
        i = 0;
        while(fgets(buf, sizeof(buf), fp))
        {
            if(++i <= 2)
                continue;
            if(p = strchr(buf, ':'))
            {
                if(strstr(buf, ifname))
                {
                    memset(pStats, 0, sizeof(*pStats));
                    if (sscanf(p+1, "%d %d %d %d %*d %*d %*d %*d %d %d %d %d %*d %*d %*d %*d",
                    &pStats->BytesReceived, &pStats->PacketsReceived, &pStats->ErrorsReceived,
                    &pStats->DiscardPacketsReceived,&pStats->BytesSent, &pStats->PacketsSent,
                    &pStats->ErrorsSent, &pStats->DiscardPacketsSent) == 8)
                    {
                        ret = 1;
                        break;
                    }
                }
            }
        }
    }

    if (fp != NULL)
        fclose(fp);

    return ret;
}
