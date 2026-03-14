#pragma once

#include <windows.h>
#include <string>

struct TestTempDir
{
    std::string path;

    TestTempDir();
    ~TestTempDir();

private:
    TestTempDir(const TestTempDir&);
    TestTempDir& operator=(const TestTempDir&);
};

bool testWriteFile(const std::string& path, const std::string& content);
bool testMakeDirectory(const std::string& path);
std::string testJoinPath(const std::string& left, const std::string& right);
bool testPathExists(const std::string& path);
std::string testExecutableDir(void);
std::string testGetEnv(const char* name);
bool testReadFile(const std::string& path, std::string& content);
std::string testFixturesDir(void);
bool testRunCommandCapture(const std::string& commandLine,
                           const std::string& workingDir,
                           std::string& output,
                           int& exitCode);
