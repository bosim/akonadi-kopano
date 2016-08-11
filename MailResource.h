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

#ifndef AIRSYNCDOWNLOADRESOURCE_H
#define AIRSYNCDOWNLOADRESOURCE_H

#include <Akonadi/ResourceBase>
#include <Akonadi/TransportResourceBase>
#include <Akonadi/ItemCreateJob>
#include <QTimer>
#include <KMime/Message>
class Session;

class MailResource : public Akonadi::ResourceBase,
                                public Akonadi::AgentBase::Observer,
				public Akonadi::TransportResourceBase
{
  Q_OBJECT

  public:
    explicit MailResource(const QString &id);
    virtual ~MailResource();

  public Q_SLOTS:
    virtual void configure(WId windowId);

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems(const Akonadi::Collection &col);
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts);
    virtual void sendItem(const Akonadi::Item &item);

  protected:
    virtual void aboutToQuit();

  private Q_SLOTS:
    void loadConfiguration();
    void errorMessageChanged(const QString &msg);
    void authRequired();
    void slotAbortRequested();

  private:  // methods
    void finish();

  private:  // members
    QTimer *intervalTimer;
    Akonadi::Collection targetCollection;
    QString password; // from wallet
    Session *session;
    QMap<KJob *, QByteArray> pendingCreateJobs;
    QList<QByteArray> mailsToDelete;
    bool downloadFinished;
    QString lastErrorMessage;
};

#endif
