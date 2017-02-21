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

#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/ItemFetchScope>

#include <MailResource.h>
#include <SettingsDialog.h>
#include <MapiSession.h>

#include "settings.h"
#include "settingsadaptor.h"

#include <akonadi/kmime/specialmailcollections.h>
#include <akonadi/kmime/messageflags.h>
#include <kwallet.h>

#include <QtDBus/QDBusConnection>
#include <QUuid>

//--------------------------------------------------------------------------------

MailResource::MailResource(const QString &id)
  : ResourceBase(id), session(0) {
  qDebug() << "Constructor";

  changeRecorder()->fetchCollection(true);
  changeRecorder()->itemFetchScope().fetchFullPayload(true);
  changeRecorder()->itemFetchScope().setFetchModificationTime(false);
  changeRecorder()->itemFetchScope().setFetchTags(true);

  new SettingsAdaptor(Settings::self());
  QDBusConnection::sessionBus()
    .registerObject(QLatin1String("/Settings"),
                    Settings::self(), QDBusConnection::ExportAdaptors);

  loadConfiguration();
  connect(this, SIGNAL(reloadConfiguration()), this, SLOT(loadConfiguration()));
  connect(this, SIGNAL(abortRequested()), this, SLOT(slotAbortRequested()));

  setNeedsNetwork(true);
  setHierarchicalRemoteIdentifiersEnabled(true);

  loadState();

  qDebug() << "Constructed";
}

//--------------------------------------------------------------------------------

MailResource::~MailResource()
{
  if(session) {
    saveState();

    delete session;
  }
}

//--------------------------------------------------------------------------------

void MailResource::loadState() {
  // Load the sync state
  QMap<QString, QByteArray> collectionsState;
  QByteArray data = QByteArray::fromBase64(Settings::self()->syncState().toAscii());

  if (!data.isEmpty()) {
    data = qUncompress(data);

    if (!data.isEmpty()) {
      QDataStream stream(data);
      stream >> collectionsState;
    }
  }

  session->setCollectionsState(collectionsState);
}

void MailResource::saveState() {
  QByteArray str;
  QDataStream dataStream(&str, QIODevice::WriteOnly);

  dataStream << session->getCollectionsState();

  Settings::self()->setSyncState(qCompress(str, 9).toBase64());
  Settings::self()->writeConfig();

}

//--------------------------------------------------------------------------------

void MailResource::retrieveCollections()
{
  qDebug() << "Retrieving collections";

  if ( status() == NotConfigured )
  {
    cancelTask(i18n("The resource is not configured correctly and can not work."));
    return;
  }

  if ( !isOnline() )
  {
    cancelTask(i18n("The resource is offline."));
    return;
  }

  KJob* job = new RetrieveCollectionsJob(name(), session);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(retrieveCollectionsResult(KJob*)));
  job->start();


}

void MailResource::retrieveCollectionsResult(KJob* job) {
  RetrieveCollectionsJob* req = qobject_cast<RetrieveCollectionsJob*>(job);
  if(req->error()) {
    QString errorString = QString("MAPI error: ") + req->errorString();
    qDebug() << "Error " << errorString;
    cancelTask(errorString);
  }
  else {
    qDebug() << "retrieveCollectionsResult job is done";
    collectionsRetrieved(req->collections);
  }
}

//--------------------------------------------------------------------------------

void MailResource::retrieveItems(const Akonadi::Collection &collection)
{
  KJob* job = new RetrieveItemsJob(collection, session);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(retrieveItemsResult(KJob*)));
  job->start();
}

void MailResource::retrieveItemsResult(KJob* job) {
  RetrieveItemsJob* req = qobject_cast<RetrieveItemsJob*>(job);
  if(req->error()) {
    QString errorString = QString("MAPI error: ") + req->errorString();
    qDebug() << "Error " << errorString;
    cancelTask(errorString);
  }
  else {
    saveState();

    qDebug() << "retrieveItemsResult job is done";
    if(req->fullSync)  {
      itemsRetrieved(req->items);
    }
    else {
      itemsRetrievedIncremental(req->items, req->deletedItems);
    }
  }
}


//--------------------------------------------------------------------------------

bool MailResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
  KJob* job = new RetrieveItemJob(item, session);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(retrieveItemResult(KJob*)));
  job->start();
  return true;
}

void MailResource::retrieveItemResult(KJob* job) {
  RetrieveItemJob* req = qobject_cast<RetrieveItemJob*>(job);
  if(req->error()) {
    QString errorString = QString("MAPI error: ") + req->errorString();
    qDebug() << "Error " << errorString;
    cancelTask(errorString);
  }
  else {
    qDebug() << "retrieveItemsResult job is done";
    itemRetrieved(req->item);
  }
}

//--------------------------------------------------------------------------------

void MailResource::itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &sourceCollection, const Akonadi::Collection &destinationCollection)
{
  KJob* job = new ItemsMovedJob(items, sourceCollection, 
				destinationCollection, session);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(itemsMovedResult(KJob*)));
  job->start();
}

void MailResource::itemsMovedResult(KJob* job) {
  ItemsMovedJob* req = qobject_cast<ItemsMovedJob*>(job);
  if(req->error()) {
    QString errorString = QString("MAPI error: ") + req->errorString();
    qDebug() << "Error " << errorString;
    cancelTask(errorString);
  }
  else {
    qDebug() << "itemsMovedResult job is done";
    changesCommitted(req->items);
  }
}

//--------------------------------------------------------------------------------

void MailResource::itemsRemoved(const Akonadi::Item::List &items)
{
  KJob* job = new ItemsRemovedJob(items, session);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(itemsRemovedResult(KJob*)));
  job->start();
}

void MailResource::itemsRemovedResult(KJob* job) {
  ItemsRemovedJob* req = qobject_cast<ItemsRemovedJob*>(job);
  if(req->error()) {
    QString errorString = QString("MAPI error: ") + req->errorString();
    qDebug() << "Error " << errorString;
    cancelTask(errorString);
  }
  else {
    qDebug() << "itemsRemovedResult job is done";
    changeProcessed(); 
  }
}

//--------------------------------------------------------------------------------

void MailResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) {
  KJob* job = new ItemAddedJob(item, collection, session);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(itemAddedResult(KJob*)));
  job->start();
  
}

void MailResource::itemAddedResult(KJob* job) {
  ItemAddedJob* req = qobject_cast<ItemAddedJob*>(job);
  if(req->error()) {
    QString errorString = QString("MAPI error: ") + req->errorString();
    qDebug() << "Error " << errorString;
    cancelTask(errorString);
  }
  else {
    qDebug() << "itemsAddedResult job is done";
    changeCommitted(req->item);
  }
}

//--------------------------------------------------------------------------------

void MailResource::itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags) {
  KJob* job = new ItemsFlagsChangedJob(items, addedFlags, removedFlags, session);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(itemsFlagsChangedResult(KJob*)));
  job->start();
}

void MailResource::itemsFlagsChangedResult(KJob* job) {
  ItemsFlagsChangedJob* req = qobject_cast<ItemsFlagsChangedJob*>(job);
  if(req->error()) {
    QString errorString = QString("MAPI error: ") + req->errorString();
    qDebug() << "Error " << errorString;
    cancelTask(errorString);
  }
  else {
    qDebug() << "itemsFlagsChanged job is done";
    changesCommitted(req->items);
  }
}


//--------------------------------------------------------------------------------

void MailResource::configure(WId windowId)
{
  QPointer<SettingsDialog> dialog = new SettingsDialog(this, windowId);

  if ( dialog->exec() == QDialog::Accepted )
  {
    loadConfiguration();
    emit configurationDialogAccepted();
  }
  else
  {
    emit configurationDialogRejected();
  }

  delete dialog;
}

//--------------------------------------------------------------------------------

void MailResource::loadConfiguration()
{
  if ( status() != Idle )  {
    // else we would delete session which is currently in use
    return;
  }

  if(session) {
    delete session;
  }

  Settings::self()->load();

  KWallet::Wallet *wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), winIdForDialogs());
  if ( wallet )
  {
    if ( wallet->setFolder(KWallet::Wallet::PasswordFolder()) )
      wallet->readPassword(identifier(), password);

    delete wallet;
  }

  if(!password.length()) {
    qDebug() << "Password is empty!";
  }

  QString server;
  server += Settings::self()->server();

  session = new Session(server, Settings::self()->userName(), password);

  // hidden propertry, probably needed when some UA filter rule on the server changes
  connect(session, SIGNAL(progress(int)), this, SIGNAL(percent(int)));
  connect(session, SIGNAL(errorMessage(QString)), this, SLOT(errorMessageChanged(QString)));
  connect(session, SIGNAL(authRequired()), this, SLOT(authRequired()));
}

//--------------------------------------------------------------------------------

void MailResource::errorMessageChanged(const QString &msg)
{
  lastErrorMessage = msg;
}

//--------------------------------------------------------------------------------

void MailResource::authRequired()
{
  QString msg = i18n("The server authentication failed. Check username and password.");
  cancelTask(msg);
  emit status(Broken, msg);
}


//--------------------------------------------------------------------------------

void MailResource::sendItem(const Akonadi::Item &item)
{
  KJob* job = new SendItemJob(item, session);
  connect(job, SIGNAL(result(KJob*)),
          this, SLOT(sendItemResult(KJob*)));
  job->start();
}

void MailResource::sendItemResult(KJob* job) {
  SendItemJob* req = qobject_cast<SendItemJob*>(job);
  if(req->error()) {
    itemSent(req->item, TransportFailed, QString());
  }
  else {
    itemSent(req->item, TransportSucceeded, QString());
  }

}

AKONADI_RESOURCE_MAIN( MailResource )

#include "MailResource.moc"
