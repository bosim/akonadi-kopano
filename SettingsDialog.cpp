/* Copyright 2015, Martin Koller, kollix@aon.at

  This file is part of airsyncDownload.

  airsyncDownload is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  airsyncDownload is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with airsyncDownload.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SettingsDialog.h>
#include <MailResource.h>
#include "settings.h"

#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <akonadi/kmime/specialmailcollections.h>
#include <akonadi/kmime/specialmailcollectionsrequestjob.h>
#include <akonadi/resourcesettings.h>

#include <KWindowSystem>
#include <KUser>
#include <kwallet.h>

using namespace Akonadi;
using namespace KWallet;

//--------------------------------------------------------------------------------

SettingsDialog::SettingsDialog(MailResource *res, WId parentWindow)
  : resource(res), wallet(0)
{
  ui.setupUi(mainWidget());

  KWindowSystem::setMainWindow(this, parentWindow);
  setWindowIcon(KIcon("network-server"));
  setWindowTitle(i18n("AirSyncDownload Account Settings"));
  setButtons(Ok|Cancel);

  loadSettings();
}

//--------------------------------------------------------------------------------

SettingsDialog::~SettingsDialog()
{
  delete wallet;
}

//--------------------------------------------------------------------------------

void SettingsDialog::loadSettings()
{
  ui.nameEdit->setText(resource->name());
  ui.nameEdit->setFocus();

  ui.serverEdit->setText(Settings::self()->server());

  ui.userNameEdit->setText(Settings::self()->userName().length() ? Settings::self()->userName() :
                           KUser().loginName());

  wallet = Wallet::openWallet(Wallet::NetworkWallet(), winId());
  if ( wallet )
  {
    if ( !wallet->hasFolder(Wallet::PasswordFolder()) )
      wallet->createFolder(Wallet::PasswordFolder());

    wallet->setFolder(Wallet::PasswordFolder());  // set current folder

    QString password;
    if ( wallet->readPassword(resource->identifier(), password) == 0 )
      ui.passwordEdit->setText(password);
  }
  else
  {
    ui.passwordEdit->setClickMessage(i18n("Wallet disabled in system settings"));
  }

  connect(ui.showPasswordCheck, SIGNAL(toggled(bool)), this, SLOT(showPasswordChecked(bool)));
}

//--------------------------------------------------------------------------------

void SettingsDialog::targetCollectionReceived(Akonadi::Collection::List collections)
{
}

//--------------------------------------------------------------------------------

void SettingsDialog::localFolderRequestJobFinished(KJob *job)
{
  if ( !job->error() )
  {
  }
}

//--------------------------------------------------------------------------------

void SettingsDialog::showPasswordChecked(bool checked)
{
  if ( checked )
    ui.passwordEdit->setEchoMode(QLineEdit::Normal);
  else
    ui.passwordEdit->setEchoMode(QLineEdit::Password);
}

//--------------------------------------------------------------------------------

void SettingsDialog::slotButtonClicked(int button)
{
  switch( button )
  {
    case Ok:
    {
      saveSettings();
      accept();
      break;
    }

    case Cancel:
    {
      reject();
      break;
    }
  }
}

//--------------------------------------------------------------------------------

void SettingsDialog::saveSettings()
{
  resource->setName(ui.nameEdit->text());

  Settings::self()->setServer(ui.serverEdit->text().trimmed());
  Settings::self()->setUserName(ui.userNameEdit->text().trimmed());

  Settings::self()->writeConfig();

  if ( wallet )
  {
    wallet->writePassword(resource->identifier(), ui.passwordEdit->text());
  }
}

//--------------------------------------------------------------------------------
