#include "rar.hpp"
#include "win32stm.cpp"

// RAR2 service header extra records.
void SetExtraInfo20(CommandData *Cmd, Archive &Arc, const std::wstring &Name)
{
    if (!Cmd->Test && (Arc.SubBlockHead.SubType == STREAM_HEAD))
        ExtractStreams20(Arc, Name.c_str());
}

// RAR3 and RAR5 service header extra records.
void SetExtraInfo(CommandData *Cmd, Archive &Arc, const std::wstring &Name)
{
    if (Arc.SubHead.CmpName(SUBHEAD_TYPE_STREAM))
        ExtractStreams(Arc, Name.c_str(), Cmd->Test);
}

// Extra data stored directly in file header.
void SetFileHeaderExtra(CommandData *Cmd, Archive &Arc, const std::wstring &Name)
{
}

// not going to...
bool ExtractSymlink(CommandData *Cmd, ComprDataIO &DataIO, Archive &Arc, const std::wstring &LinkName, bool &UpLink)
{
    return false;
}

// no way
bool ExtractHardlink(CommandData *Cmd, const std::wstring &NameNew, const std::wstring &NameExisting)
{
    return false;
}
