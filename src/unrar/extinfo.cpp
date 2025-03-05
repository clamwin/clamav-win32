#include "rar.hpp"
#include "win32stm.cpp"

// RAR2 service header extra records.
void SetExtraInfo20(CommandData *Cmd, Archive &Arc, wchar *Name)
{
    if (!Cmd->Test && (Arc.SubBlockHead.SubType == STREAM_HEAD))
        ExtractStreams20(Arc, Name);
}

// RAR3 and RAR5 service header extra records.
void SetExtraInfo(CommandData *Cmd, Archive &Arc, wchar *Name)
{
    if (Arc.SubHead.CmpName(SUBHEAD_TYPE_STREAM))
        ExtractStreams(Arc, Name, Cmd->Test);
}

// Extra data stored directly in file header.
void SetFileHeaderExtra(CommandData *Cmd, Archive &Arc, wchar *Name)
{
}

// not going to...
bool ExtractSymlink(CommandData *Cmd, ComprDataIO &DataIO, Archive &Arc, const wchar *LinkName, bool &UpLink)
{
    return false;
}

// no way
bool ExtractHardlink(CommandData *Cmd, wchar *NameNew, wchar *NameExisting, size_t NameExistingSize)
{
    return false;
}
