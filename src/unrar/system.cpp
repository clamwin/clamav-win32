#include <libclamunrar/rar.hpp>

static int SleepTime=0;

void InitSystemOptions(int SleepTime)
{
  ::SleepTime=SleepTime;
}


#if !defined(SFX_MODULE)
void SetPriority(int Priority)
{
#ifdef _WIN_ALL
  uint PriorityClass;
  int PriorityLevel;
  if (Priority<1 || Priority>15)
    return;

  if (Priority==1)
  {
    PriorityClass=IDLE_PRIORITY_CLASS;
    PriorityLevel=THREAD_PRIORITY_IDLE;

//  Background mode for Vista, can be slow for many small files.
//    if (WinNT()>=WNT_VISTA)
//      SetPriorityClass(GetCurrentProcess(),PROCESS_MODE_BACKGROUND_BEGIN);
  }
  else
    if (Priority<7)
    {
      PriorityClass=IDLE_PRIORITY_CLASS;
      PriorityLevel=Priority-4;
    }
    else
      if (Priority==7)
      {
        PriorityClass=BELOW_NORMAL_PRIORITY_CLASS;
        PriorityLevel=THREAD_PRIORITY_ABOVE_NORMAL;
      }
      else
        if (Priority<10)
        {
          PriorityClass=NORMAL_PRIORITY_CLASS;
          PriorityLevel=Priority-7;
        }
        else
          if (Priority==10)
          {
            PriorityClass=ABOVE_NORMAL_PRIORITY_CLASS;
            PriorityLevel=THREAD_PRIORITY_NORMAL;
          }
          else
          {
            PriorityClass=HIGH_PRIORITY_CLASS;
            PriorityLevel=Priority-13;
          }
  SetPriorityClass(GetCurrentProcess(),PriorityClass);
  SetThreadPriority(GetCurrentThread(),PriorityLevel);

#ifdef RAR_SMP
  ThreadPool::SetPriority(PriorityLevel);
#endif

#endif
}
#endif


// Monotonic clock. Like clock(), returns time passed in CLOCKS_PER_SEC items.
// In Android 5+ and Unix usual clock() returns time spent by all threads
// together, so we cannot use it to measure time intervals anymore.
clock_t MonoClock()
{
  return clock();
}



void Wait()
{
  if (ErrHandler.UserBreak)
    ErrHandler.Exit(RARX_USERBREAK);
#if defined(_WIN_ALL) && !defined(SFX_MODULE)
  if (SleepTime!=0)
  {
    static clock_t LastTime=MonoClock();
    if (MonoClock()-LastTime>10*CLOCKS_PER_SEC/1000)
    {
      Sleep(SleepTime);
      LastTime=MonoClock();
    }
  }
#endif
#if defined(_WIN_ALL)
  // Reset system sleep timer to prevent system going sleep.
  //SetThreadExecutionState(ES_SYSTEM_REQUIRED);
#endif
}


#if defined(_WIN_ALL)
// Load library from Windows System32 folder. Use this function to prevent
// loading a malicious code from current folder or same folder as exe.
HMODULE WINAPI LoadSysLibrary(const wchar *Name)
{
  wchar SysDir[NM];
  if (GetSystemDirectory(SysDir,ASIZE(SysDir))==0)
    return NULL;
  MakeName(SysDir,Name,SysDir,ASIZE(SysDir));
  return LoadLibrary(SysDir);
}
#endif


#ifdef USE_SSE
SSE_VERSION _SSE_Version=GetSSEVersion();

SSE_VERSION GetSSEVersion()
{
  int CPUInfo[4];
  __cpuid(CPUInfo, 0x80000000);

  // Maximum supported cpuid function. For example, Pentium M 755 returns 4 here.
  uint MaxSupported=CPUInfo[0] & 0x7fffffff;

  if (MaxSupported>=7)
  {
    __cpuid(CPUInfo, 7);
    if ((CPUInfo[1] & 0x20)!=0)
      return SSE_AVX2;
  }
  if (MaxSupported>=1)
  {
    __cpuid(CPUInfo, 1);
    if ((CPUInfo[2] & 0x80000)!=0)
      return SSE_SSE41;
    if ((CPUInfo[2] & 0x200)!=0)
      return SSE_SSSE3;
    if ((CPUInfo[3] & 0x4000000)!=0)
      return SSE_SSE2;
    if ((CPUInfo[3] & 0x2000000)!=0)
      return SSE_SSE;
  }
  return SSE_NONE;
}
#endif
