/*  akonadi-kopano
    Copyright (C) 2016 Bo Simonsen, bo@geekworld.dk

    Code based on akonadi airsync download by Martin Koller, kollix@aon.at

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <MapiSession.h>

#include <iostream>

#include <QUuid>
#include <QSslError>
#include <QDomDocument>
#include <QDateTime>

#include <KDebug>

Session::Session(const QString &server, const QString &username, const QString &password)
  : aborted(false), errorOccured(false), server(server), username(username), password(password), lpSession(NULL), lpStore(NULL), lpAddrBook(NULL), lpLogger(NULL)
{
  lpLogger = new ECLogger_Null();

  imopt_default_sending_options(&sopt);

  sopt.no_recipients_workaround = true;   // do not stop processing mail on empty recipient table
  sopt.add_received_date = true;   

  imopt_default_delivery_options(&dopt);

  // Initialize mapi system
  HRESULT hr = S_OK;
  hr = MAPIInitialize(NULL);
}

Session::~Session() {
  if ( lpStore)
    lpStore->Release();

  if (lpSession)
    lpSession->Release();

  if (lpAddrBook) 
    lpAddrBook->Release();

  MAPIUninitialize();
}

int Session::init()
{
  if(lpSession && lpStore && lpAddrBook) {
    return hrSuccess;
  }

  HRESULT hr = S_OK;

  hr = HrOpenECSession(&lpSession, "akonadi_kopano", "0", 
		       username.toStdWString().c_str(), 
		       password.toStdWString().c_str(), 
		       server.toStdString().c_str(),
                       EC_PROFILE_FLAGS_NO_NOTIFICATIONS,
                       NULL, NULL);
  if (hr != hrSuccess) {
    return hr;
  }
	
  // Get the default Store of the profile
  hr = HrOpenDefaultStore(lpSession, &lpStore);
  if (hr != hrSuccess) {
    return hr;
  }
  
  hr = lpSession->OpenAddressBook(0, NULL, AB_NO_DIALOG, &lpAddrBook);
  if (hr != hrSuccess) {
    lpLogger->Log(EC_LOGLEVEL_ERROR, "Failed to open addressbook");
    return hr;
  }

  return hr;
}


HRESULT Session::EntryIDFromSourceKey(QString const& folderSource, SBinary& entryID) {
  HRESULT hr = hrSuccess;

  if(mappingCache.contains(folderSource)) {
    kDebug() << "Cache!";
    Hex2Bin(mappingCache[folderSource], entryID);
  } else {
    SBinary folderSourceKey;
    Hex2Bin(folderSource, folderSourceKey);

    SBinary itemSourceKey;
    itemSourceKey.cb = 0;
    itemSourceKey.lpb = NULL;

    hr = RawEntryIDFromSourceKey(lpStore, folderSourceKey, 
                                 itemSourceKey, entryID);

    QString strEntryID;
    Bin2Hex(entryID, strEntryID);
    mappingCache[folderSource] = strEntryID;
  }

  return hr;
}

HRESULT Session::EntryIDFromSourceKey(QString const& folderSource, QString const& itemSource, SBinary& entryID) {
  HRESULT hr = hrSuccess;

  QString realItemSource = folderSource + ":" + itemSource;

  if(mappingCache.contains(realItemSource)) {
    kDebug() << "Cache!";
    Hex2Bin(mappingCache[realItemSource], entryID);
  } else {
    SBinary folderSourceKey;
    Hex2Bin(folderSource, folderSourceKey);

    SBinary itemSourceKey;
    Hex2Bin(itemSource, itemSourceKey);

    hr = RawEntryIDFromSourceKey(lpStore, folderSourceKey, itemSourceKey, entryID);
    
    QString strEntryID;
    Bin2Hex(entryID, strEntryID);
    mappingCache[realItemSource] = strEntryID;
  }

  return hr;
}


//--------------------------------------------------------------------------------

