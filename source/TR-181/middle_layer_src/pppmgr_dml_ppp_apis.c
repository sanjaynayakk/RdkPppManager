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

#include "pppmgr_global.h"
#include "pppmgr_data.h"
#include "pppmgr_dml_ppp_apis.h"
#include "pppmgr_dml.h"
#include <regex.h>

#define NET_STATS_FILE "/proc/net/dev"
#define PPPoE_VLAN_IF_NAME  "vlan101"
#define PPP_LCPEcho 30
#define PPP_LCPEchoRetry 3

#ifdef USE_PPP_DAEMON
#define PPPoE_LIB "/usr/lib/pppd/2.4.9/rp-pppoe.so"
#define PPPoA_LIB "/usr/lib/pppd/2.4.9/pppoatm.so"
#endif

static int CosaUtilGetIfStats(char *ifname, PDML_IF_STATS pStats);
extern char g_Subsystem[32];
extern ANSC_HANDLE bus_handle;

extern PBACKEND_MANAGER_OBJECT               g_pBEManager;

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

ANSC_STATUS PppMgr_SendPppdStartEventToQ (UINT InstanceNumber)
{
    PDML_PPP_IF_FULL pEntry=NULL;
    pEntry = PppMgr_GetIfaceData_locked(InstanceNumber);
    if (pEntry != NULL)
    {
        PPPEventQData eventData = {0};
        eventData.action = PPPMGR_EXEC_PPP_CLIENT;
        eventData.PppIfInstance = pEntry->Cfg.InstanceNumber;

        if (PppMgr_SendDataToQ(&eventData) != ANSC_STATUS_SUCCESS)
        {
            CcspTraceError(("%s %d - Failed to send data to Q\n", __FUNCTION__, __LINE__));
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_FAILURE;
        }
        pEntry->Cfg.bEnabled = true;
        CcspTraceInfo(("%s %d: posting PPPMGR_EXEC_PPP_CLIENT on Queue for %d\n", __FUNCTION__, __LINE__, eventData.PppIfInstance));
        PppMgr_GetIfaceData_release(pEntry);
        return ANSC_STATUS_SUCCESS;
    }

    return ANSC_STATUS_FAILURE;
}

bool PppMgr_EnableIf (UINT InstanceNumber, bool Enable)
{

    if (Enable == true)
    {
        CcspTraceInfo(("%s %d: Handling PPP client start for instance %d\n", __FUNCTION__, __LINE__, InstanceNumber));
        if (PppMgr_SendPppdStartEventToQ(InstanceNumber) != ANSC_STATUS_SUCCESS)
        {
            CcspTraceInfo(("%d %s: posting PPPMGR_EXEC_PPP_CLIENT event to Q failed for instance %d\n", __FUNCTION__, __LINE__, InstanceNumber));
            return false;
        }
    }
    else
    {
        CcspTraceInfo (("%s %d: disabling PPP client on instance %d\n", __FUNCTION__, __LINE__, InstanceNumber));
        if (PppMgr_StopPppClient(InstanceNumber) != ANSC_STATUS_SUCCESS)
        {
            CcspTraceInfo(("%d %s: failed to stop ppp client on instace %d\n", __FUNCTION__, __LINE__, InstanceNumber));
            return false;
        }
    }
    return true;
}

static int PppMgr_GetWanIfaceInstance (UINT InstanceNumber, int * WanIfaceInstance, int * WanVirtIfaceInstance)
{
    if (InstanceNumber <= 0 || WanIfaceInstance == NULL || WanVirtIfaceInstance == NULL)
    {
        CcspTraceError(("%s %d: Invalid args\n", __FUNCTION__, __LINE__));
        return -1;
    }

    INT iTotalNoofEntries;
    INT iTotalNoofVirtualIface;
    INT iIfaceCount;
    INT iVcount;
    char acTmpReturnValue[256] = {0};
    char acTmpQueryParam[256] = {0};
    char PppInterfacePath[128] = {0};

    if (ANSC_STATUS_FAILURE == DmlPppMgrGetParamValues(WAN_COMPONENT_NAME, WAN_DBUS_PATH, WAN_NOE_PARAM_NAME, acTmpReturnValue))
    {
        CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    // No of Wan Interfaces
    iTotalNoofEntries = atoi(acTmpReturnValue);
    CcspTraceInfo(("%s %d - TotalNoofEntries:%d\n", __FUNCTION__, __LINE__, iTotalNoofEntries));

    if (0 >= iTotalNoofEntries)
    {
        CcspTraceError(("%s %d: Cannot get number of WanInterface Entry\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    snprintf(PppInterfacePath, sizeof(PppInterfacePath), PPP_IFACE_PATH , InstanceNumber);
    CcspTraceInfo(("%s %d: comparing Wan Virtual Interfaces with %s\n", __FUNCTION__, __LINE__, PppInterfacePath));

    // for each Wan Interface
    for (iIfaceCount = 1; iIfaceCount <= iTotalNoofEntries; iIfaceCount++)
    {
        // get total no of VirtualInterface
        snprintf(acTmpQueryParam, sizeof(acTmpQueryParam), WAN_NO_OF_VIRTUAL_IFACE_PARAM_NAME, iIfaceCount);
        if (ANSC_STATUS_FAILURE == DmlPppMgrGetParamValues(WAN_COMPONENT_NAME, WAN_DBUS_PATH, acTmpQueryParam, acTmpReturnValue))
        {
            CcspTraceError(("%s %d Failed to get param value %s\n", __FUNCTION__, __LINE__, acTmpQueryParam));
            return ANSC_STATUS_FAILURE;
        }
        iTotalNoofVirtualIface = atoi (acTmpReturnValue);
        CcspTraceInfo(("%s %d - TotalNoof Wan Virtual Interface:%d\n", __FUNCTION__, __LINE__, iTotalNoofVirtualIface));

        // for each Virtual Interface
        for (iVcount = 1; iVcount <= iTotalNoofVirtualIface; iVcount++)
        {
            snprintf(acTmpQueryParam, sizeof(acTmpQueryParam), PPP_WAN_VIRTUAL_IFACE_NAME, iIfaceCount, iVcount);
            if (ANSC_STATUS_FAILURE == DmlPppMgrGetParamValues(WAN_COMPONENT_NAME, WAN_DBUS_PATH, acTmpQueryParam, acTmpReturnValue))
            {
                CcspTraceError(("%s %d Failed to get param value %s\n", __FUNCTION__, __LINE__, acTmpQueryParam));
                return ANSC_STATUS_FAILURE;
            }

            if (strcmp(acTmpReturnValue, PppInterfacePath) == 0)
            {
                *WanIfaceInstance = iIfaceCount;
                *WanVirtIfaceInstance = iVcount;
                return ANSC_STATUS_SUCCESS;
            }
        }
    }
    *WanIfaceInstance = -1;
    *WanVirtIfaceInstance = -1;
    return ANSC_STATUS_FAILURE;

}

ANSC_STATUS
PppMgr_StartPppClient (UINT InstanceNumber)
{
    int ret ;
    PDML_PPP_IF_FULL  pEntry = NULL;
    char auth_proto[64] = { 0 };
    char VLANInterfaceName[32] = { 0 };
    char command[1024] = { 0 };
    char config_command[1024] = { 0 };
    char acTmpQueryParam[256] = {0};
    int WanIfaceInstance  = -1;
    int WanVirtIfaceInstance  = -1;

    PppMgr_GetWanIfaceInstance(InstanceNumber, &WanIfaceInstance, &WanVirtIfaceInstance);
    if ((WanIfaceInstance == -1) || (WanVirtIfaceInstance == -1))
    {
        CcspTraceError(("%s %d: Unable to get Wan Iface Instance\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    pEntry = PppMgr_GetIfaceData_locked (InstanceNumber);
    {
        if(ANSC_STATUS_SUCCESS == PppMgr_checkPidExist(pEntry->Info.pppPid))
        {
            CcspTraceInfo(("pppd is already running \n"));
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_SUCCESS;
        }

        pEntry->Info.pppPid = 0;

        // get UserName & Password from hal
        platform_hal_GetPppUserName (pEntry->Cfg.Username, sizeof(pEntry->Cfg.Username));
        platform_hal_GetPppPassword (pEntry->Cfg.Password, sizeof(pEntry->Cfg.Password));

        if((strlen(pEntry->Info.Name) == 0) || (strlen(pEntry->Info.InterfaceServiceName) == 0) ||
                (strlen(pEntry->Cfg.Username) == 0) || (strlen(pEntry->Cfg.Password) == 0) ||
                (pEntry->Info.AuthenticationProtocol <= 0))
        {
            CcspTraceError(("%s %d: unable to get PPP params\n", __FUNCTION__, __LINE__));
            PppMgr_GetIfaceData_release(pEntry);
            return ANSC_STATUS_FAILURE;
        }

        switch (pEntry->Info.AuthenticationProtocol)
        {
            case DML_PPP_AUTH_PAP:
                snprintf(auth_proto, sizeof(auth_proto), " require-pap refuse-chap refuse-mschap noauth");
                break;
            case DML_PPP_AUTH_CHAP:
                snprintf(auth_proto, sizeof(auth_proto), " require-chap refuse-pap refuse-mschap noauth");
                break;
            case DML_PPP_AUTH_MS_CHAP:
                snprintf(auth_proto, sizeof(auth_proto), " require-mschap refuse-pap refuse-chap noauth");
                break;
            default:
                break;
        }

            if(pEntry->Cfg.LinkType == DML_PPPoA_LINK_TYPE)
            {
                CcspTraceInfo(("%s %d: PPPoA_LINK_TYPE: constructing arguments\n", __FUNCTION__, __LINE__));
#ifdef USE_PPP_DAEMON
                snprintf(command, sizeof(command),
                    "pppd nodetach plugin %s %s user %s password %s nodeflate %s +ipv6 usepeerdns ifname %s persist &",
                    PPPoA_LIB, pEntry->Info.InterfaceServiceName, pEntry->Cfg.Username,
                    pEntry->Cfg.Password, auth_proto, pEntry->Cfg.Alias);
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
                snprintf(command, sizeof(command),
                    "pppd nodetach plugin %s %s user %s password %s nodeflate %s +ipv6 usepeerdns ifname %s persist &",
                     PPPoE_LIB, PPPoE_VLAN_IF_NAME, pEntry->Cfg.Username, pEntry->Cfg.Password, auth_proto, pEntry->Cfg.Alias);
#else
                ret = snprintf(acTmpQueryParam, sizeof(acTmpQueryParam),"%s.%s",pEntry->Cfg.LowerLayers,"Name");
                if(ret > 0 && ret <= sizeof(config_command))
                {
                    if(ANSC_STATUS_FAILURE == DmlPppMgrGetParamValues(WAN_COMPONENT_NAME, WAN_DBUS_PATH, acTmpQueryParam, VLANInterfaceName))
                    {
                        CcspTraceError(("%s %d Failed to get param value\n", __FUNCTION__, __LINE__));
                    PppMgr_GetIfaceData_release(pEntry);
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
        CcspTraceInfo(("command to execute is  '%s'\n", command));

        system(command);

        pEntry->Info.pppPid = PppMgr_getPppPid(NULL);

        pEntry->Cfg.WanInstanceNumber = WanIfaceInstance;
        pEntry->Cfg.WanVirtIfaceInstance = WanVirtIfaceInstance;
  
        PppMgr_GetIfaceData_release(pEntry);
        }
    return ANSC_STATUS_SUCCESS;


}

ANSC_STATUS PppMgr_StopPppClient (UINT InstanceNumber)
{
    PDML_PPP_IF_FULL  pEntry = NULL;
    pEntry = PppMgr_GetIfaceData_locked(InstanceNumber);
    if (pEntry != NULL)
    {
        pEntry->Cfg.bEnabled = false;

#ifdef USE_PPP_DAEMON
        PppMgr_stopPppProcess(pEntry->Info.pppPid);
        pEntry->Info.pppPid = 0;
#else
        PppMgr_stopPppoe();
#endif

        // reset all session data
        pEntry->Info.Status = DML_IF_STATUS_Down;
        pEntry->Info.ConnectionStatus = DML_PPP_CONN_STATUS_Unconfigured;
        memset (&pEntry->Info.LocalIPAddress, 0, sizeof(pEntry->Info.LocalIPAddress));
        memset (&pEntry->Info.RemoteIPAddress, 0, sizeof(pEntry->Info.RemoteIPAddress));
        memset (&pEntry->Info.DNSServers, 0, sizeof(pEntry->Info.DNSServers));
        memset(pEntry->Info.Ip6LocalIfID,0,sizeof(pEntry->Info.Ip6LocalIfID));
        memset(pEntry->Info.Ip6RemoteIfID,0,sizeof(pEntry->Info.Ip6RemoteIfID));


        PppMgr_GetIfaceData_release(pEntry);
        pEntry = NULL;

        PppMgr_SetLinkStatusDown(InstanceNumber);
        PppMgr_SetIPCPStatusDown(InstanceNumber);
        PppMgr_SetIPv6CPStatusDown(InstanceNumber);
        return ANSC_STATUS_SUCCESS;
    }
    return ANSC_STATUS_FAILURE;
}


ANSC_STATUS PppDmlIfReset (ULONG InstanceNumber )
{
    CcspTraceInfo (("%s %d: disabling PPP client on instance %d\n", __FUNCTION__, __LINE__, InstanceNumber));
    if (PppMgr_StopPppClient(InstanceNumber) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceInfo(("%d %s: failed to stop ppp client on instace %d\n", __FUNCTION__, __LINE__, InstanceNumber));
        return false;
    }
    CcspTraceInfo(("%s %d: Handling PPP client start for instance %d\n", __FUNCTION__, __LINE__, InstanceNumber));
    if (PppMgr_SendPppdStartEventToQ(InstanceNumber) != ANSC_STATUS_SUCCESS)
    {
        CcspTraceInfo(("%d %s: posting PPPMGR_EXEC_PPP_CLIENT event to Q failed for instance %d\n", __FUNCTION__, __LINE__, InstanceNumber));
        return false;
    }
    return true;
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

    snprintf(param_value, sizeof(param_name), "%d", pEntry->Cfg.IdleDisconnectTime);
    snprintf(param_name, sizeof(param_name), PSM_PPP_IDLETIME, instancenum);
    PppMgr_RdkBus_SetParamValuesToDB(param_name,param_value);

    memset(param_value, 0, sizeof(param_value));
    memset(param_name, 0, sizeof(param_name));
	
    snprintf(param_value, sizeof(param_value), "%d", pEntry->Cfg.MaxMRUSize);
    snprintf(param_name, sizeof(param_name), PSM_PPP_MAXMRUSIZE, instancenum);
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

    // init mutex
    pthread_mutexattr_t     muttex_attr;
    pthread_mutexattr_settype(&muttex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(pEntry->mDataMutex), &(muttex_attr));

    /* Get Alias */
    snprintf(param_name, sizeof(param_name), PSM_PPP_IF_ALIAS, ulIndex);
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
    snprintf(param_name, sizeof(param_name), PSM_PPP_IF_SERVICE_NAME,ulIndex);
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
    snprintf(param_name, sizeof(param_name), PSM_PPP_IF_NAME,ulIndex);
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
    snprintf(param_name, sizeof(param_name), PSM_PPP_AUTH_PROTOCOL,ulIndex);
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
    snprintf(param_name, sizeof(param_name), PSM_PPP_LAST_COONECTION_ERROR,ulIndex);
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
    snprintf(param_name, sizeof(param_name), PSM_PPP_IDLETIME, ulIndex);
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
    snprintf(param_name, sizeof(param_name), PSM_PPP_MAXMRUSIZE, ulIndex);
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
    snprintf(param_name, sizeof(param_name), PSM_PPP_LINK_TYPE,ulIndex);
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
    snprintf(param_name, sizeof(param_name), PSM_PPP_LOWERLAYERS, ulIndex);
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
