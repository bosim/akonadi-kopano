#include <QString>

#include <mapi.h>
#include <mapiutil.h>

#include "platform.h"
#include "edkmdb.h"
#include "edkguid.h"

#include <kopano/Util.h>

HRESULT EntryIDFromSourceKey(LPMDB lpStore, SBinary const& folderSource, SBinary const& itemSource, SBinary& entryID);
HRESULT EntryIDFromSourceKey(LPMDB lpStore, QString const& folderSource, SBinary& entryID);
HRESULT EntryIDFromSourceKey(LPMDB lpStore, QString const& folderSource, QString const& itemSource, SBinary& entryID);

void Hex2Bin(QString const& input, SBinary& output);
void Bin2Hex(SBinary const& input, QString& output);
