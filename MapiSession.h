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

#include <akonadi/resourcebase.h>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <KMime/Message>

#include <kopano/stringutil.h>
#include "platform.h"

#include <mapi.h>
#include <mapiutil.h>

#include <kopano/Util.h>
#include <kopano/ECLogger.h>
#include "CommonUtil.h"
#include "inetmapi/inetmapi.h"

class Session : public QObject
{
  Q_OBJECT

  public:
    Session(const QString &server, const QString &username, const QString &password);
    ~Session();

    int fetchCollections(QString name, Akonadi::Collection::List& collections);
    int retrieveItems(Akonadi::Collection const& collection, Akonadi::Item::List& items, Akonadi::Item::List &deletedItems);
    int retrieveItem(const Akonadi::Item &item, KMime::Message::Ptr& msg);
    int sendItem(const Akonadi::Item &item);

    bool abortFetching() {
      return  true;
    }

  signals:
    void progress(int);
    void errorMessage(const QString &msg);
    void authRequired();

  private:
    int init();
    static int debugArea();
    static int debugArea2();

    void recurse(SBinary &sEntryID, Akonadi::Collection& parent, Akonadi::Collection::List& collections);
    int fetchMessage(SBinary &sEntryID, QByteArray& bodyText);

    QNetworkAccessManager *nam;
    QEventLoop *eventLoop;

    bool aborted;
    bool errorOccured;
    
    QString server;
    QString username;
    QString password;

    IMAPISession *lpSession;
    LPMDB lpStore;

    IAddrBook *lpAddrBook;
    ECLogger* lpLogger;

    sending_options sopt;
    delivery_options dopt;
};
