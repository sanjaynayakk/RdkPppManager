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
#include <syscfg/syscfg.h>
#include <dirent.h>

extern  ANSC_HANDLE bus_handle;
extern char g_Subsystem[32];
extern int sysevent_fd;
extern token_t sysevent_token;

#define NET_STATS_FILE "/proc/net/dev"
#define SYSEVENT_PPP_STATUS "ppp_status"
#define PPPOE_PROC_FILE "/proc/net/pppoe"
#define GET_PPPID_ATTEMPT    5

void get_wan_proto(wanProto_t * p_wan_proto)
{
    char          output[64] =  {0};

    if (!p_wan_proto)
        return;

    *p_wan_proto = DHCP;

    if ( !syscfg_get(NULL, "wan_proto", output, sizeof(output)) )
    {
        if (!strncmp(output, "pppoe", 5))
            *p_wan_proto = PPPOE;
    }
}

ULONG
PppGetIfAddr
    (
        char*       netdev
    )
{
    ANSC_IPV4_ADDRESS       ip4_addr = {0};

#ifdef _ANSC_LINUX

    struct ifreq            ifr;
    int                     fd = 0;

    memset(ifr.ifr_name,0,sizeof(ifr.ifr_name));
    strncpy(ifr.ifr_name, netdev,(sizeof(ifr.ifr_name)-1));

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        if (!ioctl(fd, SIOCGIFADDR, &ifr))
           memcpy(&ip4_addr.Value, ifr.ifr_ifru.ifru_addr.sa_data + 2,4);
        else {
           perror("PppGetIfAddr IOCTL failure.");
           CcspTraceWarning(("Cannot get ipv4 address of netdev:%s\n",netdev));
        }
        close(fd);
    }
    else
        perror("PppGetIfAddr failed to open socket.");

#else

    AnscGetLocalHostAddress(ip4_addr.Dot);

#endif

    return ip4_addr.Value;

}

ULONG get_ppp_ip_addr(void)
{
    ULONG addr       = 0;
    wanProto_t proto = 0;
    char buf[128]    = {0};

    /*ppp0 ip address is valid with 3 conditions: ppp0 ip address, ppp_status is "up"; wan_proto is pppoe*/

    get_wan_proto(&proto);
    if (proto != PPPOE)
        return 0;

    if (sysevent_get(sysevent_fd, sysevent_token, SYSEVENT_PPP_STATUS, buf, sizeof(buf)) == 0)
    {
        if (strncmp(buf, "up", 2) != 0)
            return 0;
    }
    else
        return 0;

    return PppGetIfAddr("ppp0");

    return 0;
}

int get_session_id(ULONG * p_id, ANSC_HANDLE hContext)
{

    FILE * fp = NULL;
    char buf[1024] = {0};
    char result[1024] = {0};
    unsigned long id = 0L;
    PDML_PPP_IF_FULL pEntry = (PDML_PPP_IF_FULL)hContext;

    if( p_id == NULL){
       CcspTraceInfo(("%s %d - Invalid Pointer p_id\n", __FUNCTION__, __LINE__));
       return 0;
    }

    if(pEntry->Cfg.LinkType == DML_PPPoE_LINK_TYPE)
    {
        if(fp = fopen(PPPOE_PROC_FILE, "r"))
        {
           /* Skip first line of /proc/net/pppoe */
           /* Id Address Device */

           while(fgets(buf, sizeof(buf)-1, fp))
           {
              id = 0L;

              if(strstr(buf, "Id") )
                 continue;

              if(sscanf(buf, "%08X", &id) == 1)
              {
                 *p_id = ntohs(id);
                 CcspTraceInfo(("PPP Session ID: %08X, %d \n", id, *p_id));
              }
           }
           fclose(fp);
        }
    }
    else if(pEntry->Cfg.LinkType == DML_PPPoA_LINK_TYPE)
    {
        char *p = NULL;
        int  id = 0;

        /*session id is only valid when ppp0 link is up*/
        if (!get_ppp_ip_addr())
            return 0;

        if (fp = fopen(LOG_FILE, "r+"))
        {
            while (fgets(buf, sizeof(buf)-1, fp))
            {
                if (strstr(buf, "pppd"))
                {
                    if (strstr(buf, "PPP session is") )
                    {
                        memset(result, 0, sizeof(result));
                        memcpy(result, buf, sizeof(result)-1);
                    }
                }
                memset(buf, 0, sizeof(buf));
            }

            /*result stores the last line for pppd session id*/
            if (result[0])
            {
                p = strstr(result, "PPP session is");
                id = 0;

                if (p)
                {
                    if (sscanf(p, "PPP session is %d", &id) == 1)
                        *p_id = id;
                }
            }

            fclose(fp);
        }
    }
    return 0;

}

int get_auth_proto(int * p_proto)
{
    FILE * fp;
    char buf[1024] = {0};
    char result[1024] = {0};

    if (fp = fopen(LOG_FILE, "r+"))
    {
        while (fgets(buf, sizeof(buf)-1, fp))
        {
            if (strstr(buf, "pppd"))
            {
                if (strstr(buf, "authentication succeeded") || strstr(buf, "authentication failed"))
                {
                    memset(result, 0, sizeof(result));
                    memcpy(result, buf, sizeof(result)-1);
                }
            }
            memset(buf, 0, sizeof(buf));
        }

        /*result stores the last line for pppd authenticatoin log*/
        if (result[0])
        {
            if (strstr(result, "CHAP"))
                *p_proto = DML_PPP_AUTH_CHAP;
            else if (strstr(result, "PAP"))
                *p_proto = DML_PPP_AUTH_PAP;
            else if (strstr(result, "MS-CHAP"))
                *p_proto = DML_PPP_AUTH_MS_CHAP;
        }

        fclose(fp);
    }

    return 0;
}

int safe_strcpy(char * dst, char * src, int dst_size)
{
    if (!dst || !src) return -1;

    memset(dst, 0, dst_size);
    strncpy(dst, src, strlen(src)<=dst_size-1 ? strlen(src):dst_size-1 );

    return 0;
}


/* ---------------------------------------------------------------------------
   This internal API will get the DUID and save it to a file
   ----------------------------------------------------------------------------*/
void PppMgr_GenerateDuidFile (char *wanName)
{

    char buff[256] = {0};

    FILE *fd = NULL;

#if defined (DUID_UUID_ENABLE)
    memset(buff, 0, sizeof(buff));
    if (syscfg_get(NULL, "UUID", uuid, sizeof(uuid)) != ANSC_STATUS_SUCCESS)
    {
        snprintf(buff, sizeof(buff), "cat %s | tr -d -", PROC_UUID_PATH);

        fd = popen(buff, "r");
        if(fd == NULL)
        {
            CcspTraceError(("%s %d: Failed to ger uuid entry\n", __FUNCTION__, __LINE__));
            return;
        }

        fgets(uuid, sizeof(uuid), fd);
        pclose(fd);

        syscfg_set(NULL, "UUID", uuid);
        syscfg_commit();
    }
#endif

    // get mac from physical interface
    char mac[64] = {0};

    memset(mac, 0, sizeof(mac));

    fd = fopen(PHY_IF_MAC_PATH, "r");
    if(fd == NULL)
    {
        CcspTraceError(("%s %d:Failed to open mac address table\n", __FUNCTION__, __LINE__));
        return;
    }

    fread(mac, sizeof(mac), 1, fd);
    fclose(fd);

    CcspTraceInfo(("%s %d: MAC of phyical interface is %s \n", __FUNCTION__, __LINE__, mac));

    // get the dhcp config path

    char file_path[256] = {0};
    struct stat st = {0};

    memset(file_path, 0, sizeof(file_path));

    snprintf(file_path, sizeof(file_path), DHCPV6_PATH, wanName);

    if (stat(file_path, &st) == -1)
    {
        // directory does not exists, so create it
        mkdir(file_path, 0644);
        CcspTraceInfo(("%s %d: created directory %s\n", __FUNCTION__, __LINE__, file_path));
    }

    strncat (file_path, DHCPV6_DUID_FILE, sizeof(file_path) - strlen(file_path) - 1);

    // wite duid in duid-client file
    FILE * fp_duid = fopen(file_path, "w");
    if (fp_duid == NULL)
    {
        CcspTraceError(("%s %d: cannot open file:%s due to %s\n", __FUNCTION__, __LINE__, file_path, strerror(errno)))
            return;
    }

    memset(buff, 0, sizeof(buff));

    sprintf(buff, DUID_TYPE);
#if defined (DUID_UUID_ENABLE)
    sprintf(buff+4, uuid);
#else
    sprintf(buff+6, HW_TYPE);
    sprintf(buff+12, mac);
#endif
    fprintf(fp_duid, "%s", buff);
    fclose(fp_duid);

}

void PppMgr_RemoveDuidFile (char *wanName)
{

    struct stat st = {0};
    char file_path[256] = {0};

    memset(file_path, 0, sizeof(file_path));

    snprintf(file_path, sizeof(file_path), DHCPV6_PATH DHCPV6_DUID_FILE, wanName);

    if (stat(file_path, &st) != -1)
    {
        // directory does not exists, so create it
        CcspTraceInfo(("%s %d: deleting  %s\n", __FUNCTION__, __LINE__, file_path));
        remove(file_path);
    }

}

int set_syscfg(char *pValue,char *param)
{
    if((syscfg_set(NULL, param, pValue) != 0))
    {
        return -1;
    }
    else
    {
        if(syscfg_commit() != 0)
        {
            return -1;
        }
        return 0;
    }
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

    return pid;
}

int PppMgr_getIfaceDataWithPid (pid_t pid)
{

    PDML_PPP_IF_FULL pEntry=NULL; 
    int InstanceNumber =  -1 ;
    int totalNoOfPppIface = DmlGetTotalNoOfPPPInterfaces(NULL);

    for (int i =1; i <= totalNoOfPppIface; i++)
    {
        pEntry = PppMgr_GetIfaceData_locked(i);
        if (pEntry != NULL)
        {
            if (pEntry->Info.pppPid == pid)
            {
                InstanceNumber = (int)pEntry->Cfg.InstanceNumber;
                PppMgr_GetIfaceData_release(pEntry);
                break;
            }
            PppMgr_GetIfaceData_release(pEntry);
            pEntry = NULL;
        }
    }

    return InstanceNumber;

}
