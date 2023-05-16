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

#ifndef  _PPPMGR_DML_PPP_APIS_H_
#define  _PPPMGR_DML_PPP_APIS_H_

#include "pppmgr_dml.h"
#include "utctx_api.h"
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <utapi_util.h>
#include <mqueue.h>

#define MAXINSTANCE    128
#define DNS_FILE "/var/run/ppp/resolv.conf"
#define LOG_FILE "/var/log/messages"

#define  IREP_FOLDER_NAME_PPPIF             "PPPIf"
#define  DML_RR_NAME_PPPIFAlias             "Alias"
#define  DML_RR_NAME_PPPIFInsNum            "InstanceNumber"
#define  DML_RR_NAME_PPPIFNextInsNunmber    "NextInstanceNumber"
#define  DML_RR_NAME_PPPIFNewlyAdded        "NewlyAdded"

#define UP  "Up"
#define DOWN  "Down"
#define PPP_IPCP_STATUS_DOWN    DOWN
#define PPP_IPV6CP_STATUS_DOWN  DOWN
#define PHY_IF_MAC_PATH       "/sys/class/net/atm0/address"
#define DHCPV6_PATH           "/etc/dibbler/%s/"
#define DHCPV6_DUID_FILE      "client-duid"
#if defined (DUID_UUID_ENABLE)
#define DUID_TYPE "0004"  /* duid-type duid-uuid 4 */
#else
#define DUID_TYPE "00:03:"  /* duid-type duid-ll 3 */
#define HW_TYPE "00:01:"    /* hw type is always 1 */
#endif

#define WAN_COMPONENT_NAME    "eRT.com.cisco.spvtg.ccsp.wanmanager"
#define WAN_DBUS_PATH    "/com/cisco/spvtg/ccsp/wanmanager"
/* PPP wan manager params */
#define PPP_IFACE_PATH  "Device.PPP.Interface.%d"
#define PPP_WAN_VIRTUAL_IFACE_NAME  "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.%d.PPP.Interface"
#define WAN_NO_OF_VIRTUAL_IFACE_PARAM_NAME    "Device.X_RDK_WanManager.Interface.%d.VirtualInterfaceNumberOfEntries"
#if defined(WAN_MANAGER_UNIFICATION_ENABLED)
#define PPP_LCP_STATUS_PARAM_NAME    "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.%d.PPP.Status"
#define PPP_LINK_STATUS_PARAM_NAME    "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.%d.PPP.Status"
#define PPP_IPCP_STATUS_PARAM_NAME    "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.%d.PPP.IPCPStatus"
#define PPP_IPV6CP_STATUS_PARAM_NAME    "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.%d.PPP.IPv6CPStatus"
/* wan manager params */
#define WAN_NOE_PARAM_NAME    "Device.X_RDK_WanManager.InterfaceNumberOfEntries"
#define WAN_PHY_PATH_PARAM_NAME    "Device.X_RDK_WanManager.Interface.%d.BaseInterface"
#define WAN_IFACE_NAME             "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.1.Name"
#else
#define PPP_LCP_STATUS_PARAM_NAME    "Device.X_RDK_WanManager.CPEInterface.%d.PPP.LCPStatus"
#define PPP_LINK_STATUS_PARAM_NAME    "Device.X_RDK_WanManager.CPEInterface.%d.PPP.LinkStatus"
#define PPP_IPCP_STATUS_PARAM_NAME    "Device.X_RDK_WanManager.CPEInterface.%d.PPP.IPCPStatus"
#define PPP_IPV6CP_STATUS_PARAM_NAME    "Device.X_RDK_WanManager.CPEInterface.%d.PPP.IPv6CPStatus"
/* wan manager params */
#define WAN_NOE_PARAM_NAME    "Device.X_RDK_WanManager.CPEInterfaceNumberOfEntries"
#define WAN_PHY_PATH_PARAM_NAME    "Device.X_RDK_WanManager.CPEInterface.%d.Phy.Path"
#define WAN_IFACE_NAME             "Device.X_RDK_WanManager.CPEInterface.%d.Wan.Name"
#endif /* WAN_MANAGER_UNIFICATION_ENABLED */

#define DATAMODEL_PARAM_LENGTH    256

#define PANDM_COMPONENT_NAME  "eRT.com.cisco.spvtg.ccsp.pam"
#define PANDM_DBUS_PATH       "/com/cisco/spvtg/ccsp/pam"

/* DeviceInfo Params */
#define UP_TIME_PARAM_NAME        "Device.DeviceInfo.UpTime"

typedef enum _eventHandlingAction_
{
    PPPMGR_BUS_SET,
    PPPMGR_EXEC_PPP_CLIENT
} PPPMGR_EVENT_ACTION;

typedef struct _PPPEventQData_ {
    PPPMGR_EVENT_ACTION action;
    INT PppIfInstance;
    char * keyPath;     // DML object pattern
    char * comPath;     // component path
    char * busPath;     // bus path
    char val[64];
} PPPEventQData;


ANSC_STATUS PppDmlGetIfEntry( ANSC_HANDLE hContext, ULONG ulIndex, PDML_PPP_IF_FULL pEntry );

ANSC_STATUS PppDmlGetIntfValuesFromPSM( ANSC_HANDLE hContext, ULONG  ulIndex, PDML_PPP_IF_FULL pEntry );

ANSC_STATUS PppDmlSetIfValues ( ANSC_HANDLE hContext, ULONG ulIndex, ULONG ulInstanceNumber, char* pAlias );

ANSC_STATUS PPPDmlGetIfInfo( ANSC_HANDLE hContext, ULONG ulInstanceNumber, PDML_PPP_IF_INFO pInfo );

ANSC_STATUS PppDmlSetIfCfg    ( ANSC_HANDLE hContext, PDML_PPP_IF_CFG pCfg );

ANSC_STATUS PppDmlGetIfStats ( ANSC_HANDLE hContext, ULONG ulPppIfInstanceNumber, PDML_IF_STATS pStats, PDML_PPP_IF_FULL pEntry );

ANSC_STATUS PppDmlIfReset( ULONG ulInstanceNumber);

ULONG PppGetIfAddr( char* netdev );

ANSC_STATUS  PppMgr_checkPidExist( pid_t pppPid );

ANSC_STATUS PppMgr_stopPppProcess( pid_t pid );

ANSC_STATUS PppMgr_stopPppoe(void);

ANSC_STATUS DmlWanmanagerSetParamValues( const char *pComponent, const char *pBus,
        const char *pParamName, const char *pParamVal, enum dataType_e type, unsigned int bCommitFlag );

static ANSC_STATUS DmlPppMgrGetParamValues(char *pComponent, char *pBus, char *pParamName, char *pReturnVal);

ULONG GetUptimeinSeconds ();

int PppMgr_RdkBus_SetParamValuesToDB( char *pParamName, char *pParamVal );

int validateUsername( char* pString);

ULONG DmlGetTotalNoOfPPPInterfaces ( ANSC_HANDLE hContext);

void PppMgr_GenerateDuidFile (char *wanName);

void PppMgr_RemoveDuidFile (char *wanName);

DML_PPP_IF_FULL  * PppMgr_GetIfaceData_locked (UINT pppIfaceInstance);

void PppMgr_GetIfaceData_release (DML_PPP_IF_FULL * pPppTable);

ANSC_STATUS PppMgr_StopPppClient (UINT InstanceNumber);

int PppMgr_getIfaceDataWithPid (pid_t pid);
#endif
