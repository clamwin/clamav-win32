#include "rar.hpp"

static int SleepTime = 0;

void InitSystemOptions(int SleepTime)
{
  ::SleepTime = SleepTime;
}

void SetPriority(int Priority)
{
  uint PriorityClass;
  int PriorityLevel;
  if (Priority < 1 || Priority > 15)
    return;

  if (Priority == 1)
  {
    PriorityClass = IDLE_PRIORITY_CLASS;
    PriorityLevel = THREAD_PRIORITY_IDLE;

    //  Background mode for Vista, can be slow for many small files.
    //    if (WinNT()>=WNT_VISTA)
    //      SetPriorityClass(GetCurrentProcess(),PROCESS_MODE_BACKGROUND_BEGIN);
  }
  else if (Priority < 7)
  {
    PriorityClass = IDLE_PRIORITY_CLASS;
    PriorityLevel = Priority - 4;
  }
  else if (Priority == 7)
  {
    PriorityClass = BELOW_NORMAL_PRIORITY_CLASS;
    PriorityLevel = THREAD_PRIORITY_ABOVE_NORMAL;
  }
  else if (Priority < 10)
  {
    PriorityClass = NORMAL_PRIORITY_CLASS;
    PriorityLevel = Priority - 7;
  }
  else if (Priority == 10)
  {
    PriorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
    PriorityLevel = THREAD_PRIORITY_NORMAL;
  }
  else
  {
    PriorityClass = HIGH_PRIORITY_CLASS;
    PriorityLevel = Priority - 13;
  }
  SetPriorityClass(GetCurrentProcess(), PriorityClass);
  SetThreadPriority(GetCurrentThread(), PriorityLevel);

#ifdef RAR_SMP
  ThreadPool::SetPriority(PriorityLevel);
#endif
}

// Monotonic clock. Like clock(), returns time passed in CLOCKS_PER_SEC items.
// In Android 5+ and Unix usual clock() returns time spent by all threads
// together, so we cannot use it to measure time intervals anymore.
clock_t MonoClock()
{
  return clock();
}

typedef EXECUTION_STATE(WINAPI *imp_SetThreadExecutionState)(EXECUTION_STATE esFlags);

void Wait()
{
  if (ErrHandler.UserBreak)
    ErrHandler.Exit(RARX_USERBREAK);

  if (SleepTime != 0)
  {
    static clock_t LastTime = MonoClock();
    if (MonoClock() - LastTime > 10 * CLOCKS_PER_SEC / 1000)
    {
      static clock_t LastTime = MonoClock();
      if (MonoClock() - LastTime > 10 * CLOCKS_PER_SEC / 1000)
      {
        Sleep(SleepTime);
        LastTime = MonoClock();
      }
    }

    // Reset system sleep timer to prevent system going sleep.
    HMODULE kernel32 = GetModuleHandle(TEXT("kernel32"));
    if (kernel32)
    {
      imp_SetThreadExecutionState pSetThreadExecutionState = (imp_SetThreadExecutionState)GetProcAddress(kernel32, "SetThreadExecutionState");
      if (pSetThreadExecutionState)
        pSetThreadExecutionState(ES_SYSTEM_REQUIRED);
    }
  }
}

// Load library from Windows System32 folder. Use this function to prevent
// loading a malicious code from current folder or same folder as exe.
HMODULE WINAPI LoadSysLibrary(const wchar *Name)
{
  std::vector<wchar> SysDir(MAX_PATH);
  if (GetSystemDirectory(SysDir.data(), (UINT)SysDir.size()) == 0)
    return nullptr;
  std::wstring FullName;
  MakeName(SysDir.data(), Name, FullName);
  return LoadLibrary(FullName.c_str());
}
