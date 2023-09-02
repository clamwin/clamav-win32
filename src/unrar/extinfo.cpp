#include <libclamunrar/rar.hpp>

void SetFileHeaderExtra(CommandData *Cmd, Archive &Arc, wchar *Name)
{
}

void SetExtraInfo(CommandData *Cmd, Archive &Arc, wchar *Name)
{
}

void SetExtraInfo20(CommandData *Cmd, Archive &Arc, wchar *Name)
{
}

bool ExtractSymlink(CommandData* Cmd, ComprDataIO& DataIO, Archive& Arc, const wchar* LinkName, bool& UpLink)
{
    return false;
}

bool ExtractHardlink(CommandData* Cmd, wchar* NameNew, wchar* NameExisting, size_t NameExistingSize)
{
    return false;
}

bool LinksToDirs(const wchar* SrcName, const wchar* SkipPart, std::wstring& LastChecked)
{
    return false;
}