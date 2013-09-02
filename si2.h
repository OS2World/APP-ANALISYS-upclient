#ifdef LOGINNAME
#undef LOGINNAME
#endif
#ifdef PASSWORD
#undef PASSWORD
#endif
#ifdef HOSTID
#undef HOSTID
#endif
#define LOGINNAME os2loginname
#define PASSWORD os2password
#define HOSTID  os2hostid
#define CPUS 4
#define CMD_KI_RDCNT          (0x63)
#define LL2F(high, low)       (4294967296.0*(high)+(low))
APIRET APIENTRY DosPerfSysCall(ULONG ulCommand, ULONG ulParm1,
                               ULONG ulParm2, ULONG ulParm3);
typedef struct _CPUUTIL
{
    ULONG ulTimeLow;     /* Low 32 bits of time stamp      */
    ULONG ulTimeHigh;    /* High 32 bits of time stamp     */
    ULONG ulIdleLow;     /* Low 32 bits of idle time       */
    ULONG ulIdleHigh;    /* High 32 bits of idle time      */
    ULONG ulBusyLow;     /* Low 32 bits of busy time       */
    ULONG ulBusyHigh;    /* High 32 bits of busy time      */
    ULONG ulIntrLow;     /* Low 32 bits of interrupt time  */
    ULONG ulIntrHigh;    /* High 32 bits of interrupt time */
} CPUUTIL;

typedef CPUUTIL *PCPUUTIL;

