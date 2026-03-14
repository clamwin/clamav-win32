#include "test_support.h"

#include <stdio.h>
#include <shlwapi.h>
#include <vector>

namespace
{
void deleteTree(const std::string& path)
{
    std::string pattern = path;
    if (!pattern.empty() && pattern[pattern.size() - 1] != '\\' && pattern[pattern.size() - 1] != '/')
        pattern += "\\";
    pattern += "*";

    WIN32_FIND_DATAA data;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
                continue;

            std::string child = path;
            if (!child.empty() && child[child.size() - 1] != '\\' && child[child.size() - 1] != '/')
                child += "\\";
            child += data.cFileName;

            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                deleteTree(child);
                RemoveDirectoryA(child.c_str());
            }
            else
            {
                DeleteFileA(child.c_str());
            }
        }
        while (FindNextFileA(hFind, &data));

        FindClose(hFind);
    }
}
}

TestTempDir::TestTempDir()
{
    char tempPath[MAX_PATH];
    char tempFile[MAX_PATH];
    tempPath[0] = '\0';
    tempFile[0] = '\0';
    GetTempPathA(MAX_PATH, tempPath);
    GetTempFileNameA(tempPath, "cwg", 0, tempFile);
    DeleteFileA(tempFile);
    CreateDirectoryA(tempFile, NULL);
    path = tempFile;
}

TestTempDir::~TestTempDir()
{
    if (!path.empty())
    {
        deleteTree(path);
        RemoveDirectoryA(path.c_str());
    }
}

bool testWriteFile(const std::string& path, const std::string& content)
{
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp)
        return false;

    size_t written = fwrite(content.data(), 1, content.size(), fp);
    fclose(fp);
    return written == content.size();
}

bool testMakeDirectory(const std::string& path)
{
    if (CreateDirectoryA(path.c_str(), NULL) != 0)
        return true;
    return GetLastError() == ERROR_ALREADY_EXISTS;
}

std::string testJoinPath(const std::string& left, const std::string& right)
{
    if (left.empty())
        return right;
    if (right.empty())
        return left;

    std::string joined = left;
    char tail = joined[joined.size() - 1];
    if (tail != '\\' && tail != '/')
        joined += "\\";
    joined += right;
    return joined;
}

bool testPathExists(const std::string& path)
{
    return !path.empty() && GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::string testExecutableDir(void)
{
    char modulePath[MAX_PATH];
    modulePath[0] = '\0';
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);
    char* slash = strrchr(modulePath, '\\');
    if (slash)
        *slash = '\0';
    return modulePath;
}

std::string testGetEnv(const char* name)
{
    if (!name || !*name)
        return std::string();

    char buf[2048];
    DWORD len = GetEnvironmentVariableA(name, buf, sizeof(buf));
    if (len == 0 || len >= sizeof(buf))
        return std::string();
    return buf;
}

bool testReadFile(const std::string& path, std::string& content)
{
    content.clear();
    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp)
        return false;

    char buffer[512];
    size_t readCount = 0;
    while ((readCount = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        content.append(buffer, buffer + readCount);

    fclose(fp);
    return true;
}

std::string testFixturesDir(void)
{
    char path[MAX_PATH];
    path[0] = '\0';
    GetModuleFileNameA(NULL, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    return testJoinPath(testJoinPath(testJoinPath(path, ".."), "src"), "clamwin-gui-cpp\\tests\\fixtures");
}

bool testRunCommandCapture(const std::string& commandLine,
                           const std::string& workingDir,
                           std::string& output,
                           int& exitCode)
{
    output.clear();
    exitCode = -1;

    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE readPipe = NULL;
    HANDLE writePipe = NULL;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
        return false;

    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = writePipe;
    si.hStdError = writePipe;

    std::vector<char> cmd(commandLine.begin(), commandLine.end());
    cmd.push_back('\0');

    BOOL created = CreateProcessA(NULL,
                                  cmd.data(),
                                  NULL,
                                  NULL,
                                  TRUE,
                                  CREATE_NO_WINDOW,
                                  NULL,
                                  workingDir.empty() ? NULL : workingDir.c_str(),
                                  &si,
                                  &pi);

    CloseHandle(writePipe);
    writePipe = NULL;

    if (!created)
    {
        CloseHandle(readPipe);
        return false;
    }

    char buffer[512];
    DWORD bytesRead = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
        output.append(buffer, buffer + bytesRead);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD processExitCode = 0;
    GetExitCodeProcess(pi.hProcess, &processExitCode);
    exitCode = (int)processExitCode;

    CloseHandle(readPipe);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}
