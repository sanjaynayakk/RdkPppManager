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

#include "pppmgr_data.h"
#include "pppmgr_dml_ppp_apis.h"
#include "pppmgr_global.h"
#include "pppmgr_dml.h"

extern void * g_pDslhDmlAgent;
extern PBACKEND_MANAGER_OBJECT               g_pBEManager;

/**********************************************************************

    caller:     owner of the object

    prototype:

        ANSC_HANDLE
        BackEndManagerCreate
            (
            );

    description:

        This function constructs datamodel object and return handle.

    argument:

    return:     newly created qos object.

**********************************************************************/

ANSC_HANDLE
BackEndManagerCreate
    (
        VOID
    )
{
    ANSC_STATUS                returnStatus = ANSC_STATUS_SUCCESS;
    PBACKEND_MANAGER_OBJECT    pMyObject    = (PBACKEND_MANAGER_OBJECT)NULL;

    /*
        * We create object by first allocating memory for holding the variables and member functions.
        */
    pMyObject = (PBACKEND_MANAGER_OBJECT) AnscAllocateMemory(sizeof(BACKEND_MANAGER_OBJECT));

    if (!pMyObject)
    {
        return  (ANSC_HANDLE)NULL;
    }
    pMyObject->hPPP = (PDATAMODEL_PPP) AnscAllocateMemory(sizeof(DATAMODEL_PPP));

    /*
     * Initialize the common variables and functions for a container object.
     */
    pMyObject->Oid               = DATAMODEL_BASE_OID;
    pMyObject->Create            = BackEndManagerCreate;
    pMyObject->Remove            = BackEndManagerRemove;
    pMyObject->Initialize        = BackEndManagerInitialize;

    return  (ANSC_HANDLE)pMyObject;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        BackEndManagerRemove
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function remove manager object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
BackEndManagerRemove
    (
        ANSC_HANDLE                 hThisObject
    )
{
    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PBACKEND_MANAGER_OBJECT         pMyObject    = (PBACKEND_MANAGER_OBJECT)hThisObject;

    if (pMyObject == NULL)
    {
        AnscTraceError(("%s:%d:: Pointer is null!!\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    AnscFreeMemory((ANSC_HANDLE)pMyObject->hPPP);

    /* Remove self */
    AnscFreeMemory((ANSC_HANDLE)pMyObject);

    return returnStatus;
}

/**********************************************************************

    caller:     self

    prototype:

        ANSC_STATUS
        BackEndManagerInitialize
            (
                ANSC_HANDLE                 hThisObject
            );

    description:

        This function initiate manager object and return handle.

    argument:   ANSC_HANDLE                 hThisObject
            This handle is actually the pointer of this object
            itself.

    return:     operation status.

**********************************************************************/

ANSC_STATUS
BackEndManagerInitialize
    (
        ANSC_HANDLE                 hThisObject
    )
{

    ANSC_STATUS                     returnStatus = ANSC_STATUS_SUCCESS;
    PBACKEND_MANAGER_OBJECT  pMyObject    = (PBACKEND_MANAGER_OBJECT)hThisObject;
    ULONG                           ulEntryCount        = 0;
    ULONG                           ulIndex             = 0;
    ULONG                           ulNextInsNum        = 0;

    if (pMyObject == NULL)
    {
        AnscTraceError(("%s:%d:: Pointer is null!!\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
    PDATAMODEL_PPP           pPpp         = (PDATAMODEL_PPP)pMyObject->hPPP;

    if (pPpp == NULL)
    {
        AnscTraceError(("%s:%d:: Pointer is null!!\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }

    /* Initiation Device.PPP.Interface */
    ulEntryCount = DmlGetTotalNoOfPPPInterfaces(NULL);

    ulNextInsNum = 1;

    for ( ulIndex = 0; ulIndex < ulEntryCount; ulIndex++ )
    {
        PppDmlGetIfEntry(NULL, ulIndex, &pPpp->PppTable[ulIndex]);

        if ( pPpp->PppTable[ulIndex].Cfg.InstanceNumber == 0 )
    {
            pPpp->PppTable[ulIndex].Cfg.InstanceNumber = ulNextInsNum;

            PppDmlSetIfValues(NULL, ulIndex, ulNextInsNum, pPpp->PppTable[ulIndex].Info.Name);

            ulNextInsNum++;
    }
    else
    {
            ulNextInsNum = pPpp->PppTable[ulIndex].Cfg.InstanceNumber + 1;
        }
    }

    return returnStatus;

}

DML_PPP_IF_FULL  * PppMgr_GetIfaceData_locked (UINT pppIfaceInstance)
{
    if (pppIfaceInstance <= 0 || DmlGetTotalNoOfPPPInterfaces(NULL) < pppIfaceInstance)
    {
        return NULL;
    }

    UINT iface_index = pppIfaceInstance - 1;

    PDATAMODEL_PPP             pMyObject               = (PDATAMODEL_PPP      )g_pBEManager->hPPP;
    PDML_PPP_IF_FULL           pPppTable                  = (PDML_PPP_IF_FULL    )pMyObject->PppTable;

    pthread_mutex_lock(&pPppTable[iface_index].mDataMutex);
    return &pPppTable[iface_index];
}

void PppMgr_GetIfaceData_release (DML_PPP_IF_FULL * pPppTable)
{
    if (pPppTable != NULL)
    {
        pthread_mutex_unlock(&pPppTable->mDataMutex);
    }
}



