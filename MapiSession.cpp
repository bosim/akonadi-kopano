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

//--------------------------------------------------------------------------------

int Session::debugArea()
{
  static int num = KDebug::registerArea("kopano_resource"); 
  return num;
}

//--------------------------------------------------------------------------------

int Session::debugArea2()
{
  static int num = KDebug::registerArea("kopano_resource2"); 
  return num;
}

//--------------------------------------------------------------------------------

#define PRINT_DEBUG(x) { kDebug(debugArea()) << QDateTime::currentDateTime().toString(Qt::ISODate) << x; }
#define PRINT_DEBUG2(x) { kDebug(debugArea2()) << QDateTime::currentDateTime().toString(Qt::ISODate) << x; }

//--------------------------------------------------------------------------------

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
  HRESULT hr = S_OK;

  hr = HrOpenECSession(lpLogger, &lpSession, "test", "0", 
		       username.toStdWString().c_str(), 
		       password.toStdWString().c_str(), 
		       server.toStdString().c_str());
  if (hr != hrSuccess) {
    return -1;
  }
	
  // Get the default Store of the profile
  hr = HrOpenDefaultStore(lpSession, &lpStore);
  if (hr != hrSuccess) {
    return -1;
  }
  
  hr = lpSession->OpenAddressBook(0, NULL, AB_NO_DIALOG, &lpAddrBook);
  if (hr != hrSuccess) {
    lpLogger->Log(EC_LOGLEVEL_ERROR, "Failed to open addressbook");
    return -1;
  }

  return 0;
}

void Session::recurse(SBinary &sEntryID, Akonadi::Collection& parent, Akonadi::Collection::List& collections) {
  IMAPIFolder *lpFolder = NULL;	
  HRESULT hr = S_OK;
  ULONG ulObjType;
  IMAPITable *lpTable = NULL;
  LPSRowSet lpRowSet = NULL;
  enum { EID, NAME, IMAPID, SUBFOLDERS, CONTAINERCLASS, NUM_COLS };
  SizedSPropTagArray(NUM_COLS, spt) = { NUM_COLS, {PR_ENTRYID, PR_DISPLAY_NAME_W, PR_EC_IMAP_ID, PR_SUBFOLDERS, PR_CONTAINER_CLASS_A } };
  char* strEntryID;
  char folderName[255];
  QStringList contentTypes;

  contentTypes << KMime::Message::mimeType() 
	       << Akonadi::Collection::mimeType();

  hr = lpSession->OpenEntry(sEntryID.cb, (LPENTRYID) sEntryID.lpb, &IID_IMAPIFolder, 0, &ulObjType, (LPUNKNOWN *) &lpFolder);
  if (hr != hrSuccess)
    goto exit;
  
  hr = lpFolder->GetHierarchyTable(0, &lpTable);
  if (hr != hrSuccess)
    goto exit;

  hr = lpTable->SetColumns((LPSPropTagArray) &spt, 0);
  if (hr != hrSuccess)
    goto exit;

  hr = lpTable->QueryRows(50, 0, &lpRowSet);
  if (hr != hrSuccess)
    goto exit;
	
  for (ULONG i = 0; i < lpRowSet->cRows; ++i) {
    Akonadi::Collection collection;

    if (PROP_TYPE(lpRowSet->aRow[i].lpProps[NAME].ulPropTag) == PT_UNICODE) {
      wcstombs(folderName, lpRowSet->aRow[i].lpProps[NAME].Value.lpszW, 255);
      Util::bin2hex(lpRowSet->aRow[i].lpProps[EID].Value.bin.cb, 
		    lpRowSet->aRow[i].lpProps[EID].Value.bin.lpb, 
		    &strEntryID, NULL);

      PRINT_DEBUG("Got " << folderName);
      PRINT_DEBUG("EntryID " << strEntryID);
      collection.setName(folderName);
      collection.setRemoteId(strEntryID);
      collection.setContentMimeTypes(contentTypes);
      collection.setParentCollection(parent);

      collections.append(collection);
    }
    if (PROP_TYPE(lpRowSet->aRow[i].lpProps[SUBFOLDERS].ulPropTag) == PT_BOOLEAN) {
      recurse(lpRowSet->aRow[i].lpProps[EID].Value.bin, collection, collections);
    }
  }

exit:	
  if (lpRowSet)
    FreeProws(lpRowSet);
  
  if (lpTable)
    lpTable->Release();

  return;
}

int Session::fetchCollections(QString name, Akonadi::Collection::List& collections) 
{
  LPSPropValue lpPropVal = NULL;
  HRESULT hr = S_OK;
  SBinary sEntryID;
  QStringList contentTypes;

  contentTypes << KMime::Message::mimeType() 
	       << Akonadi::Collection::mimeType();

  if ( init() != 0 )
    return -1;

  PRINT_DEBUG("fetchCollections");

  hr = HrGetOneProp(lpStore, PR_IPM_SUBTREE_ENTRYID, &lpPropVal);
  if (hr != hrSuccess) {
    return -1;
  }

  sEntryID = lpPropVal->Value.bin;

  Akonadi::Collection root;
  root.setName(name);
  root.setRemoteId("/");
  root.setContentMimeTypes(contentTypes);
  root.setParentCollection(Akonadi::Collection::root());
  collections.append(root);

  recurse(sEntryID, root, collections);

  return collections.size();
}

int Session::fetchMessage(SBinary &sEntryID, QByteArray& bodyText) {
  HRESULT hr = S_OK;
  LPMESSAGE lpMessage = NULL;
  LPSTREAM lpStream = NULL;
  ULONG ulObjType;
  std::string strMessage;
  char *szMessage = NULL;

  hr = lpStore->OpenEntry(sEntryID.cb, (LPENTRYID) sEntryID.lpb, &IID_IMessage, MAPI_DEFERRED_ERRORS, &ulObjType, (LPUNKNOWN *) &lpMessage);
		  
  if (hr != hrSuccess) {
    goto exit;
  }

  hr = lpMessage->OpenProperty(PR_EC_IMAP_EMAIL, &IID_IStream, 0, 0, (LPUNKNOWN*)&lpStream);

  if (hr == hrSuccess) {
    hr = Util::HrStreamToString(lpStream, strMessage);
    if (hr == hrSuccess) {
      strMessage = strMessage.c_str();
    }
  } 
  else {
    hr = IMToINet(lpSession, lpAddrBook, lpMessage, &szMessage, sopt, lpLogger);
    if (hr != hrSuccess) {
      lpLogger->Log(EC_LOGLEVEL_ERROR, "Error converting MAPI to MIME: 0x%08x", hr);
      goto exit;
    }
    strMessage = szMessage;
  }   
  
exit:
  bodyText = strMessage.c_str();
  
  return 0;

}

int Session::retrieveItems(Akonadi::Collection const& collection, Akonadi::Item::List& items, Akonadi::Item::List &deletedItems) {
  IMAPIFolder *lpFolder = NULL;	
  HRESULT hr = S_OK;
  ULONG ulObjType;
  IMAPITable *lpTable = NULL;
  LPSRowSet lpRowSet = NULL;
  ULONG ulRows = 0;
  SBinary sEntryID;
  char* strEntryID;
  enum { EID, SIZE, NUM_COLS };
  SizedSPropTagArray(NUM_COLS, spt) = { NUM_COLS, {PR_ENTRYID, PR_MESSAGE_SIZE} };

  if ( init() != 0 )
    return -1;

  if(collection.remoteId() == "/")
    return 0;

  Util::hex2bin(collection.remoteId().toStdString().c_str(), 
		strlen(collection.remoteId().toStdString().c_str()),
		&sEntryID.cb, &sEntryID.lpb, NULL);

  hr = lpStore->OpenEntry(sEntryID.cb, (LPENTRYID) sEntryID.lpb, 
			  &IID_IMAPIFolder, 0, &ulObjType, 
			  (LPUNKNOWN *)&lpFolder);

  if (hr != hrSuccess) {
    goto exit;
  }
	
  hr = lpFolder->GetContentsTable(0, &lpTable);
  if (hr != hrSuccess) {
    goto exit;
  }

  // Set the columns of the table
  hr = lpTable->SetColumns((LPSPropTagArray)&spt, 0);
  if (hr != hrSuccess) {
    goto exit;
  }

  while(TRUE) {
    // returns 50 rows from the table, beginning at the current cursor position
    hr = lpTable->QueryRows(50, 0, &lpRowSet);
    if (hr != hrSuccess)
      goto exit;

    if (lpRowSet->cRows == 0)
      break;

    for (int i = 0; i < lpRowSet->cRows; i++) {

      Akonadi::Item item;

      Util::bin2hex(lpRowSet->aRow[i].lpProps[EID].Value.bin.cb, 
		    lpRowSet->aRow[i].lpProps[EID].Value.bin.lpb, 
		    &strEntryID, NULL);
      
      PRINT_DEBUG("Got " << strEntryID);

      item.setParentCollection(collection);
      item.setRemoteId(strEntryID);
      item.setRemoteRevision(QString::number(1));

      items << item;
      
    }

    ulRows += lpRowSet->cRows;

    // Free rows and reset variable
    FreeProws(lpRowSet);
    lpRowSet = NULL;
  }

 exit:
  return items.count();

}

int Session::retrieveItem(const Akonadi::Item &item, KMime::Message::Ptr& msg) {
  SBinary sEntryID;

  Util::hex2bin(item.remoteId().toStdString().c_str(), 
		strlen(item.remoteId().toStdString().c_str()),
		&sEntryID.cb, &sEntryID.lpb, NULL);

  QByteArray bodyText;

  fetchMessage(sEntryID, bodyText);

  msg->setContent(KMime::CRLFtoLF(bodyText));
  msg->parse();

  return 0;
}


//--------------------------------------------------------------------------------

int Session::sendItem(const Akonadi::Item &item) {
  HRESULT hr = S_OK;
  LPSPropValue lpPropVal = NULL;
  ULONG ulObjType;
  IMAPIFolder *lpFolder = NULL;
  LPMESSAGE lpMessage = 0;
  SPropValue propName;

  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();
  QByteArray bodyText = msg->encodedContent(true);

  if ( init() != 0 )
    return -1;

  hr = HrGetOneProp(lpStore, PR_IPM_OUTBOX_ENTRYID, &lpPropVal);
  if (hr != hrSuccess) {
    return -1;
  }

  hr = lpSession->OpenEntry(lpPropVal->Value.bin.cb, (LPENTRYID) lpPropVal->Value.bin.lpb, NULL, MAPI_MODIFY, &ulObjType, (LPUNKNOWN *) &lpFolder);
  if (hr != hrSuccess) {
    return -1;
  }

  hr = lpFolder->CreateMessage(NULL, 0, &lpMessage);        
  if (hr != hrSuccess) {
    return -1;
  }

  hr = IMToMAPI(lpSession, lpStore, lpAddrBook, lpMessage, bodyText.constData(), dopt, lpLogger);
  if (hr != hrSuccess) {
    return -1;
  }

  propName.ulPropTag = PR_DELETE_AFTER_SUBMIT;
  propName.Value.b = TRUE;
  lpMessage->SetProps(1, &propName, NULL);

  propName.ulPropTag = PR_MESSAGE_CLASS;
  propName.Value.LPSZ = const_cast<TCHAR *>(_T("IPM.Note"));
  lpMessage->SetProps(1, &propName, NULL);

  lpMessage->SubmitMessage(0);

  return 0;
}
