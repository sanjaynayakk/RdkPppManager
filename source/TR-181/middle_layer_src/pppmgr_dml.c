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
#include "pppmgr_dml.h"
#include "pppmgr_dml_apis.h"
#include "pppmgr_dml_ppp_apis.h"
#include "ccsp_psm_helper.h"
#include <syscfg/syscfg.h>

#define PPP_IF_SERVICE_NAME "atm0.0.0.38"
#define PPP_IF_NAME         "pppoa0"
#define WAN_IF_NAME         "wan0"
#define PPPoE_VLAN_IF_NAME  "vlan101"

extern char g_Subsystem[32];
extern ANSC_HANDLE bus_handle;

extern PBACKEND_MANAGER_OBJECT               g_pBEManager;



/***********************************************************************

 APIs for Object:

    PPP.

    *  PPP_GetParamBoolValue
    *  PPP_GetParamIntValue
    *  PPP_GetParamUlongValue
    *  PPP_GetParamStringValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPP_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPP_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPP_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPP_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPP_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPP_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        PPP_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
PPP_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    /* check the parameter name and return the corresponding value */
    if (AnscEqualString(ParamName, "SupportedNCPs", TRUE))
    {
        ULONG flag;
        
        PppDmlGetSupportedNCPs(NULL, &flag);

        AnscCopyString(pValue, "");

        if (flag & DML_PPP_SUPPORTED_NCP_ATCP)
        {
            strcat(pValue, "ATCP,");
        }

        if (flag & DML_PPP_SUPPORTED_NCP_IPCP)
        {
            strcat(pValue, "IPCP,");
        }

        if (flag & DML_PPP_SUPPORTED_NCP_IPXCP)
        {
            strcat(pValue, "IPXCP,");
        }

        if (flag & DML_PPP_SUPPORTED_NCP_NBFCP)
        {
            strcat(pValue, "NBFCP,");
        }

        if (flag & DML_PPP_SUPPORTED_NCP_IPv6CP)
        {
            strcat(pValue, "IPv6CP,");
        }

        return 0;
    }

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return -1;
}


/***********************************************************************

 APIs for Object:

    PPP.Interface.{i}.

    *  Interface_GetEntryCount
    *  Interface_GetEntry
    *  Interface_AddEntry
    *  Interface_DelEntry
    *  Interface_GetParamBoolValue
    *  Interface_GetParamIntValue
    *  Interface_GetParamUlongValue
    *  Interface_GetParamStringValue
    *  Interface_SetParamBoolValue
    *  Interface_SetParamIntValue
    *  Interface_SetParamUlongValue
    *  Interface_SetParamStringValue
    *  Interface_Validate
    *  Interface_Commit
    *  Interface_Rollback

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Interface_GetEntryCount
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to retrieve the count of the table.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The count of the table

**********************************************************************/
ULONG
Interface_GetEntryCount
    (
        ANSC_HANDLE                 hInsContext
    )
{
      return DmlGetTotalNoOfPPPInterfaces(NULL);
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ANSC_HANDLE
        Interface_GetEntry
            (
                ANSC_HANDLE                 hInsContext,
                ULONG                       nIndex,
                ULONG*                      pInsNumber
            );

    description:

        This function is called to retrieve the entry specified by the index.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                ULONG                       nIndex,
                The index of this entry;

                ULONG*                      pInsNumber
                The output instance number;

    return:     The handle to identify the entry

**********************************************************************/
ANSC_HANDLE
Interface_GetEntry
    (
        ANSC_HANDLE                 hInsContext,
        ULONG                       nIndex,
        ULONG*                      pInsNumber
    )
{
    PDATAMODEL_PPP             pMyObject               = (PDATAMODEL_PPP      )g_pBEManager->hPPP;
    PDML_PPP_IF_FULL           pPppTable                  = (PDML_PPP_IF_FULL    )pMyObject->PppTable;

    pthread_mutex_lock(&pPppTable[nIndex].mDataMutex);
    pPppTable[nIndex].Cfg.InstanceNumber = nIndex + 1;
    *pInsNumber = nIndex + 1;
    pthread_mutex_unlock(&pPppTable[nIndex].mDataMutex);

    return &(pPppTable[nIndex]);
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Interface_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Interface_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    BOOL retStatus = FALSE;

    pthread_mutex_lock(&pEntry->mDataMutex);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Enable", TRUE))
    {
        /* collect value */
        *pBool = pEntry->Cfg.bEnabled;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "IPCPEnable", TRUE))
    {
        /* collect value */
        *pBool = pEntry->Cfg.bIPCPEnabled;

        retStatus = TRUE;
    }
    
    if( AnscEqualString(ParamName, "IPv6CPEnable", TRUE))
    {
        /* collect value */
        *pBool = pEntry->Cfg.bIPV6CPEnabled;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "Reset", TRUE))
    {
        /* collect value */
        *pBool = FALSE;

        retStatus = TRUE;
    }
    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Interface_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Interface_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    BOOL retStatus = FALSE;

    pthread_mutex_lock(&pEntry->mDataMutex);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Status", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Info.Status;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "LastChange", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = AnscGetTimeIntervalInSeconds(pEntry->Info.LastChange, GetUptimeinSeconds());

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "ConnectionStatus", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Info.ConnectionStatus;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "X_T_ONLINE_DE_SRU", TRUE))
    {

        *puLong =  pEntry->Info.SRU;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "X_T_ONLINE_DE_SRD", TRUE))
    {

        *puLong =  pEntry->Info.SRD;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "LastConnectionError", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Info.LastConnectionError;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "AutoDisconnectTime", TRUE))
    {
        /* collect value */
        *puLong = pEntry->Cfg.AutoDisconnectTime;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "IdleDisconnectTime", TRUE))
    {
        /* collect value */
        *puLong = pEntry->Cfg.IdleDisconnectTime;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "WarnDisconnectDelay", TRUE))
    {
        /* collect value */
        *puLong = pEntry->Cfg.WarnDisconnectDelay;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "EncryptionProtocol", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Info.EncryptionProtocol;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "CompressionProtocol", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Info.CompressionProtocol;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "AuthenticationProtocol", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Info.AuthenticationProtocol;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "MaxMRUSize", TRUE))
    {
        /* collect value */
        *puLong = pEntry->Cfg.MaxMRUSize;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "CurrentMRUSize", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);
        pEntry->Info.CurrentMRUSize = pEntry->Cfg.MaxMRUSize;
        *puLong = pEntry->Info.CurrentMRUSize;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "ConnectionTrigger", TRUE))
    {
        /* collect value */
        *puLong = pEntry->Cfg.ConnectionTrigger;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "LCPEcho", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Info.LCPEcho;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "LCPEchoRetry", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Info.LCPEchoRetry;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_LinkType", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Cfg.LinkType;

        retStatus = TRUE;
    }

    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Interface_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
Interface_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;
    uint32_t retStatus = -1;

    pthread_mutex_lock(&pEntry->mDataMutex);

    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "Alias", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pEntry->Cfg.Alias);

        retStatus = 0;
    }

    if( AnscEqualString(ParamName, "Name", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        AnscCopyString(pValue, pEntry->Info.Name);

        retStatus = 0;
    }

    if( AnscEqualString(ParamName, "LowerLayers", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pEntry->Cfg.LowerLayers);

        retStatus = 0;
    }

    if( AnscEqualString(ParamName, "Username", TRUE))
    {
		 /* collect value */
        AnscCopyString(pValue, pEntry->Cfg.Username);

        retStatus = 0;
    }

    if( AnscEqualString(ParamName, "Password", TRUE))
    {
		/* collect value */
        AnscCopyString(pValue, "");

        retStatus = 0;
    }
    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Interface_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Interface_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;
    bool retStatus = TRUE;

    if(pEntry == NULL)
    {
        return FALSE;
    }

    pthread_mutex_lock(&pEntry->mDataMutex);    

    if( AnscEqualString(ParamName, "Enable", TRUE))
    {
        /* save update to backup */
        retStatus = PppMgr_EnableIf(pEntry->Cfg.InstanceNumber, bValue);
    }

    if( AnscEqualString(ParamName, "IPCPEnable", TRUE))
    {
        /* save update to backup */
        pEntry->Cfg.bIPCPEnabled = bValue;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "IPv6CPEnable", TRUE))
    {
        /* save update to backup */
        pEntry->Cfg.bIPV6CPEnabled = bValue;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "Reset", TRUE))
    {
        /* save update to backup */
        if ( pEntry->Cfg.bEnabled && bValue )
        {
            retStatus = PppDmlIfReset(pEntry->Cfg.InstanceNumber);
        }
        else
        {
            retStatus = FALSE;
        }
    }

    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Interface_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Interface_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Interface_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Interface_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    BOOL retStatus = FALSE;
    char buf[128] = {0};

    if(pEntry == NULL)
    {
        return FALSE;
    }

    pthread_mutex_lock(&pEntry->mDataMutex);

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "AutoDisconnectTime", TRUE))
    {
        /* save update to backup */
        pEntry->Cfg.AutoDisconnectTime = uValue;
        snprintf(buf, sizeof(buf), "sleep %ld && pppoe-stop &",pEntry->Cfg.AutoDisconnectTime);
        system(buf);
        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "IdleDisconnectTime", TRUE))
    {
        /* save update to backup */
        pEntry->Cfg.IdleDisconnectTime = uValue;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "WarnDisconnectDelay", TRUE))
    {
        /* save update to backup */
        pEntry->Cfg.WarnDisconnectDelay = uValue;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "MaxMRUSize", TRUE))
    {
        /* save update to backup */
#ifdef _COSA_DRG_CNS_
        /*not supported*/
        retStatus = FALSE;
#else
        pEntry->Cfg.MaxMRUSize = uValue;
        retStatus = TRUE;
#endif
    }

    if( AnscEqualString(ParamName, "ConnectionTrigger", TRUE))
    {
        /* save update to backup */
#ifdef _COSA_DRG_CNS_
        /*don't support manual*/
        if (uValue == PPP_DML_CONN_TRIGGER_Manual) 
        {
            pthread_mutex_unlock(&pEntry->Info.mDataMutex);

            return FALSE;
        }
#endif
        pEntry->Cfg.ConnectionTrigger = uValue;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "X_RDK_LinkType", TRUE))
    {
        pEntry->Cfg.LinkType = uValue;

        retStatus = TRUE;
    }

    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Interface_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Interface_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    BOOL retStatus = FALSE;
    int ret = 0;

    if(pEntry == NULL)
    {
        return FALSE;
    }

    pthread_mutex_lock(&pEntry->mDataMutex);

    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "Alias", TRUE))
    {
        /* save update to backup */
        AnscCopyString(pEntry->Cfg.Alias, pString);

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "LowerLayers", TRUE))
    {
        /* save update to backup */
        AnscCopyString(pEntry->Cfg.LowerLayers, pString);

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "Username", TRUE))
    {
        if(strcmp(pString,"") != 0)
        {
            ret = validateUsername(pString);
            if (ret !=0)
            {
                retStatus = FALSE;
            }
            else
            {
                AnscCopyString(pEntry->Cfg.Username, pString);
                retStatus = TRUE;
            }
        }
    }

    if( AnscEqualString(ParamName, "Password", TRUE))
    {
        if(strcmp(pString,"") != 0)
        {
            AnscCopyString(pEntry->Cfg.Password, pString);

            retStatus = TRUE;
        }
    }

    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Interface_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
Interface_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{

    PDATAMODEL_PPP             pMyObject         = (PDATAMODEL_PPP      )g_pBEManager->hPPP;
    PDML_PPP_IF_FULL           pEntry            = (PDML_PPP_IF_FULL    )hInsContext;
    ULONG                      ulIndex                 = 0;
    ULONG                      ulEntryCount;


    ulEntryCount = DmlGetTotalNoOfPPPInterfaces(NULL);

    for ( ulIndex = 0; ulIndex < ulEntryCount; ulIndex++ )
    {
        if ( 
                ((ULONG)pEntry != (ULONG)&pMyObject->PppTable[ulIndex]) &&
                AnscEqualString(pEntry->Info.Name, pMyObject->PppTable[ulIndex].Info.Name, TRUE)
           )
        {
            AnscCopyString(pReturnParamName, "Name");

            *puLength = AnscSizeOfString("Name");
            
            return FALSE;
        }
    }

    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Interface_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Interface_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL    )hInsContext;
    pthread_mutex_lock(&pEntry->mDataMutex);
    PppDmlSetIfCfg(NULL, &pEntry->Cfg);
    pthread_mutex_unlock(&pEntry->mDataMutex);    

    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        Interface_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
Interface_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;
    
    pthread_mutex_lock(&pEntry->mDataMutex);

    PppDmlGetIfCfg(NULL, &pEntry->Cfg);

    pthread_mutex_unlock(&pEntry->mDataMutex);
    
    return 0;
}

/***********************************************************************

 APIs for Object:

    PPP.Interface.{i}.PPPoE.

    *  PPPoE_GetParamBoolValue
    *  PPPoE_GetParamIntValue
    *  PPPoE_GetParamUlongValue
    *  PPPoE_GetParamStringValue
    *  PPPoE_SetParamBoolValue
    *  PPPoE_SetParamIntValue
    *  PPPoE_SetParamUlongValue
    *  PPPoE_SetParamStringValue
    *  PPPoE_Validate
    *  PPPoE_Commit
    *  PPPoE_Rollback

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPPoE_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPPoE_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPPoE_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPPoE_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPPoE_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPPoE_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    BOOL retStatus = FALSE;

    pthread_mutex_lock(&pEntry->mDataMutex);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "SessionID", TRUE))
    {
        /* collect value */
        PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);

        *puLong = pEntry->Info.SessionID;

        retStatus = TRUE;

    }
    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        PPPoE_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
PPPoE_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    uint32_t retSuccess = 0;

    pthread_mutex_lock(&pEntry->mDataMutex);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "ACName", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pEntry->Cfg.ACName);
    }

    if( AnscEqualString(ParamName, "ServiceName", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pEntry->Cfg.ServiceName);
    }
    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retSuccess;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPPoE_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPPoE_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPPoE_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPPoE_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPPoE_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPPoE_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPPoE_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
PPPoE_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    BOOL retStatus = FALSE;

    pthread_mutex_lock(&pEntry->mDataMutex);
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "ACName", TRUE))
    {
        /* save update to backup */
        AnscCopyString(pEntry->Cfg.ACName, pString);

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "ServiceName", TRUE))
    {
        /* save update to backup */
        AnscCopyString(pEntry->Cfg.ServiceName, pString);

        retStatus = TRUE;
    }
    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        PPPoE_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
PPPoE_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        PPPoE_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
PPPoE_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL    )hInsContext;

    pthread_mutex_lock(&pEntry->mDataMutex);
    PppDmlSetIfCfg(NULL, &pEntry->Cfg);
    pthread_mutex_unlock(&pEntry->mDataMutex);
   
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        PPPoE_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
PPPoE_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    pthread_mutex_lock(&pEntry->mDataMutex);

    PppDmlGetIfCfg(NULL, &pEntry->Cfg);

    pthread_mutex_unlock(&pEntry->mDataMutex);
    
    return 0;
}

/***********************************************************************

 APIs for Object:

    PPP.Interface.{i}.IPCP.

    *  IPCP_GetParamBoolValue
    *  IPCP_GetParamIntValue
    *  IPCP_GetParamUlongValue
    *  IPCP_GetParamStringValue
    *  IPCP_SetParamBoolValue
    *  IPCP_SetParamIntValue
    *  IPCP_SetParamUlongValue
    *  IPCP_SetParamStringValue
    *  IPCP_Validate
    *  IPCP_Commit
    *  IPCP_Rollback

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPCP_GetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL*                       pBool
            );

    description:

        This function is called to retrieve Boolean parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL*                       pBool
                The buffer of returned boolean value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPCP_GetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL*                       pBool
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    pthread_mutex_lock(&pEntry->mDataMutex);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "PassthroughEnable", TRUE))
    {
        /* collect value */
        *pBool = pEntry->Cfg.PassthroughEnable;

        pthread_mutex_unlock(&pEntry->mDataMutex);

        return TRUE;
    }
    pthread_mutex_unlock(&pEntry->mDataMutex);

    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPCP_GetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int*                        pInt
            );

    description:

        This function is called to retrieve integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int*                        pInt
                The buffer of returned integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPCP_GetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int*                        pInt
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPCP_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPCP_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    /* check the parameter name and return the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        IPCP_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 1023 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/
ULONG
IPCP_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    uint32_t retStatus = -1;

    pthread_mutex_lock(&pEntry->mDataMutex);

    PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "LocalIPAddress", TRUE))
    {
        /* collect value */
        sprintf(pValue,
                "%u.%u.%u.%u",
                pEntry->Info.LocalIPAddress.Dot[0],
                pEntry->Info.LocalIPAddress.Dot[1],
                pEntry->Info.LocalIPAddress.Dot[2],
                pEntry->Info.LocalIPAddress.Dot[3]
               );
        retStatus = 0;
    }

    if( AnscEqualString(ParamName, "RemoteIPAddress", TRUE))
    {
        /* collect value */
        sprintf(pValue,
                "%u.%u.%u.%u",
                pEntry->Info.RemoteIPAddress.Dot[0],
                pEntry->Info.RemoteIPAddress.Dot[1],
                pEntry->Info.RemoteIPAddress.Dot[2],
                pEntry->Info.RemoteIPAddress.Dot[3]
               );
        retStatus = 0;
    }

    if( AnscEqualString(ParamName, "DNSServers", TRUE))
    {
        /* collect value */
        if (!pEntry->Info.DNSServers[1].Value)
            sprintf(pValue,
                    "%u.%u.%u.%u",
                    pEntry->Info.DNSServers[0].Dot[0],
                    pEntry->Info.DNSServers[0].Dot[1],
                    pEntry->Info.DNSServers[0].Dot[2],
                    pEntry->Info.DNSServers[0].Dot[3]
                   );
        else
            sprintf(pValue,
                    "%u.%u.%u.%u,%u.%u.%u.%u",
                    pEntry->Info.DNSServers[0].Dot[0],
                    pEntry->Info.DNSServers[0].Dot[1],
                    pEntry->Info.DNSServers[0].Dot[2],
                    pEntry->Info.DNSServers[0].Dot[3],
                    pEntry->Info.DNSServers[1].Dot[0],
                    pEntry->Info.DNSServers[1].Dot[1],
                    pEntry->Info.DNSServers[1].Dot[2],
                    pEntry->Info.DNSServers[1].Dot[3]
                   );
        retStatus = 0;
    }

    if( AnscEqualString(ParamName, "PassthroughDHCPPool", TRUE))
    {
        /* collect value */
        AnscCopyString(pValue, pEntry->Cfg.PassthroughDHCPPool);

        retStatus = 0;
    }
    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPCP_SetParamBoolValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                BOOL                        bValue
            );

    description:

        This function is called to set BOOL parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                BOOL                        bValue
                The updated BOOL value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPCP_SetParamBoolValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        BOOL                        bValue
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    BOOL retStatus = FALSE;

    pthread_mutex_lock(&pEntry->mDataMutex);
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "PassthroughEnable", TRUE))
    {
        /* save update to backup */
#ifdef _COSA_DRG_CNS_
        /*not supported*/
        retStatus = FALSE;
#else
        pEntry->Cfg.PassthroughEnable = bValue;
    
        retStatus = TRUE;
#endif
    }

    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPCP_SetParamIntValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                int                         iValue
            );

    description:

        This function is called to set integer parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                int                         iValue
                The updated integer value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPCP_SetParamIntValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        int                         iValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPCP_SetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG                       uValue
            );

    description:

        This function is called to set ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG                       uValue
                The updated ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPCP_SetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG                       uValue
    )
{
    /* check the parameter name and set the corresponding value */

    /* CcspTraceWarning(("Unsupported parameter '%s'\n", ParamName)); */
    return FALSE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPCP_SetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pString
            );

    description:

        This function is called to set string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pString
                The updated string value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
IPCP_SetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pString
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;
 
    BOOL retStatus = FALSE;

    if(pEntry == NULL)
    {
        CcspTraceError(("%s -%d : Invalid Memory, Failed to set Parameter Value\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    pthread_mutex_lock(&pEntry->mDataMutex);
    /* check the parameter name and set the corresponding value */
    if( AnscEqualString(ParamName, "PassthroughDHCPPool", TRUE))
    {
        /* save update to backup */
#ifdef _COSA_DRG_CNS_
        /*not supported*/
        retStatus = FALSE;
#else
        AnscCopyString(pEntry->Cfg.PassthroughDHCPPool, pString);

        retStatus = TRUE;
#endif
    }

    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;

}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        IPCP_Validate
            (
                ANSC_HANDLE                 hInsContext,
                char*                       pReturnParamName,
                ULONG*                      puLength
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       pReturnParamName,
                The buffer (128 bytes) of parameter name if there's a validation. 

                ULONG*                      puLength
                The output length of the param name. 

    return:     TRUE if there's no validation.

**********************************************************************/
BOOL
IPCP_Validate
    (
        ANSC_HANDLE                 hInsContext,
        char*                       pReturnParamName,
        ULONG*                      puLength
    )
{
    return TRUE;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        IPCP_Commit
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to finally commit all the update.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
IPCP_Commit
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL    )hInsContext;

    pthread_mutex_lock(&pEntry->mDataMutex);

    PppDmlSetIfCfg(NULL, &pEntry->Cfg);

    pthread_mutex_unlock(&pEntry->mDataMutex);
    
    return 0;
}

/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        IPCP_Rollback
            (
                ANSC_HANDLE                 hInsContext
            );

    description:

        This function is called to roll back the update whenever there's a 
        validation found.

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

    return:     The status of the operation.

**********************************************************************/
ULONG
IPCP_Rollback
    (
        ANSC_HANDLE                 hInsContext
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    pthread_mutex_lock(&pEntry->mDataMutex);

    PppDmlGetIfCfg(NULL, &pEntry->Cfg);

    pthread_mutex_unlock(&pEntry->mDataMutex);
    
    return 0;
}

/***********************************************************************

 APIs for Object:

    PPP.Interface.{i}.IPv6CP.

    * IPv6CP_GetParamStringValue 

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        ULONG
        IPv6CP_GetParamStringValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                char*                       pValue,
                ULONG*                      pUlSize
            );

    description:

        This function is called to retrieve string parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                char*                       pValue,
                The string value buffer;

                ULONG*                      pUlSize
                The buffer of length of string value;
                Usually size of 45 will be used.
                If it's not big enough, put required size here and return 1;

    return:     0 if succeeded;
                1 if short of buffer size; (*pUlSize = required size)
                -1 if not supported.

**********************************************************************/

ULONG
IPv6CP_GetParamStringValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        char*                       pValue,
        ULONG*                      pUlSize
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    uint32_t retStatus = -1;
    
    pthread_mutex_lock(&pEntry->mDataMutex);

    PPPDmlGetIfInfo(hInsContext, pEntry->Cfg.InstanceNumber, &pEntry->Info);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "LocalInterfaceIdentifier", TRUE))
    {
        sprintf(pValue, pEntry->Info.Ip6LocalIfID);

        retStatus = 0;
    }
    else if ( AnscEqualString(ParamName, "RemoteInterfaceIdentifier", TRUE)) 
    {
        sprintf(pValue, pEntry->Info.Ip6RemoteIfID);

        retStatus = 0;
    }
    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;
}


/***********************************************************************

 APIs for Object:

    PPP.Interface.{i}.Stats.

    *  Stats_GetParamUlongValue

***********************************************************************/
/**********************************************************************  

    caller:     owner of this object 

    prototype: 

        BOOL
        Stats_GetParamUlongValue
            (
                ANSC_HANDLE                 hInsContext,
                char*                       ParamName,
                ULONG*                      puLong
            );

    description:

        This function is called to retrieve ULONG parameter value; 

    argument:   ANSC_HANDLE                 hInsContext,
                The instance handle;

                char*                       ParamName,
                The parameter name;

                ULONG*                      puLong
                The buffer of returned ULONG value;

    return:     TRUE if succeeded.

**********************************************************************/
BOOL
Stats_GetParamUlongValue
    (
        ANSC_HANDLE                 hInsContext,
        char*                       ParamName,
        ULONG*                      puLong
    )
{
    PDML_PPP_IF_FULL           pEntry                  = (PDML_PPP_IF_FULL)hInsContext;

    DML_IF_STATS               Stats = {0};
    BOOL retStatus = FALSE;

    PppDmlGetIfStats(NULL, pEntry->Cfg.InstanceNumber, &Stats, pEntry);

    pthread_mutex_lock(&pEntry->mDataMutex);
    /* check the parameter name and return the corresponding value */
    if( AnscEqualString(ParamName, "BytesSent", TRUE))
    {
        /* collect value */
        *puLong = Stats.BytesSent;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "BytesReceived", TRUE))
    {
        /* collect value */
        *puLong = Stats.BytesReceived;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "PacketsSent", TRUE))
    {
        /* collect value */
        *puLong = Stats.PacketsSent;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "PacketsReceived", TRUE))
    {
        /* collect value */
        *puLong = Stats.PacketsReceived;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "ErrorsSent", TRUE))
    {
        /* collect value */
        *puLong = Stats.ErrorsSent;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "ErrorsReceived", TRUE))
    {
        /* collect value */
        *puLong = Stats.ErrorsReceived;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "UnicastPacketsSent", TRUE))
    {
        /* collect value */
        *puLong = Stats.UnicastPacketsSent;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "UnicastPacketsReceived", TRUE))
    {
        /* collect value */
        *puLong = Stats.UnicastPacketsReceived;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "DiscardPacketsSent", TRUE))
    {
        /* collect value */
        *puLong = Stats.DiscardPacketsSent;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "DiscardPacketsReceived", TRUE))
    {
        /* collect value */
        *puLong = Stats.DiscardPacketsReceived;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "MulticastPacketsSent", TRUE))
    {
        /* collect value */
        *puLong = Stats.MulticastPacketsSent;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "MulticastPacketsReceived", TRUE))
    {
        /* collect value */
        *puLong = Stats.MulticastPacketsReceived;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "BroadcastPacketsSent", TRUE))
    {
        /* collect value */
        *puLong = Stats.BroadcastPacketsSent;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "BroadcastPacketsReceived", TRUE))
    {
        /* collect value */
        *puLong = Stats.BroadcastPacketsReceived;

        retStatus = TRUE;
    }

    if( AnscEqualString(ParamName, "UnknownProtoPacketsReceived", TRUE))
    {
        /* collect value */
        *puLong = Stats.UnknownProtoPacketsReceived;

        retStatus = TRUE;
    }
    pthread_mutex_unlock(&pEntry->mDataMutex);

    return retStatus;

}

