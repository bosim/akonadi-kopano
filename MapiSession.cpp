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

#include "RetrieveItemsJob.h"

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
  if(lpSession) {
    return 0;
  }

  HRESULT hr = S_OK;

  hr = HrOpenECSession(lpLogger, &lpSession, "test", "0", 
		       username.toStdWString().c_str(), 
		       password.toStdWString().c_str(), 
		       server.toStdString().c_str(),
                       0,
                       NULL, NULL);
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




//--------------------------------------------------------------------------------

