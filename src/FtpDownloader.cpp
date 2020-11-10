/****************************************************************************
 **
 ** Portions Copyright (C) 2012 Research In Motion Limited.
 ** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Research In Motion Ltd. (http://www.rim.com/company/contact/)
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the examples of the BB10 Platform and is derived
 ** from a similar file that is part of the Qt Toolkit.
 **
 ** You may use this file under the terms of the BSD license as follows:
 **
 ** "Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are
 ** met:
 **   * Redistributions of source code must retain the above copyright
 **     notice, this list of conditions and the following disclaimer.
 **   * Redistributions in binary form must reproduce the above copyright
 **     notice, this list of conditions and the following disclaimer in
 **     the documentation and/or other materials provided with the
 **     distribution.
 **   * Neither the name of Research In Motion, nor the name of Nokia
 **     Corporation and its Subsidiary(-ies), nor the names of its
 **     contributors may be used to endorse or promote products
 **     derived from this software without specific prior written
 **     permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 **
 ****************************************************************************/

#include "FtpDownloader.hpp"
#include "Common.hpp"

#include <bb/cascades/Button>
#include <bb/cascades/Label>
#include <bb/cascades/ListView>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/TextField>

#include <QtCore/QFile>

using namespace bb::cascades;

namespace
{
    const QString hostnameSetting = QLatin1String("hostname");
    const QString usernameSetting = QLatin1String("username");
    const QString passwordSetting = QLatin1String("password");
}

FtpDownloader::FtpDownloader(QObject *parent)
    : QObject(parent)
    , m_hostname(Common::getAppSetting(hostnameSetting))
    , m_username(Common::getAppSetting(usernameSetting))
    , m_password(Common::getAppSetting(passwordSetting))
    , m_ftp(0)
    , m_file(0)
    , m_parentDirectoryAvailable(false)
    , m_downloadPossible(false)
    , m_connectPossible(true)
    , m_connectLabel(Common::qstr("Conectar"))
    , m_selectionPossible(false)
    , m_waitIndicatorPossible(false)
    , m_networkSession(0)
{
    setInfoStatusLabel("Não conectado.");

    m_selectionPossible = false;
    emit selectionPossibleChanged();

    m_waitIndicatorPossible = false;
    emit waitIndicatorPossibleChanged();

    m_parentDirectoryAvailable = false;
    emit parentDirectoryAvailableChanged();

    m_downloadPossible = false;
    emit downloadPossibleChanged();

    connect(&m_progressDialogController, SIGNAL(canceled()), this, SLOT(cancelDownload()));
}

FtpDownloader::~FtpDownloader()
{
    delete m_file;
    m_file = 0;
}

void FtpDownloader::connectOrDisconnect()
{
    if (m_ftp) {
        m_ftp->abort();
        m_ftp->deleteLater();
        m_ftp = 0;

        m_model.clear();

        m_selectionPossible = false;
        emit selectionPossibleChanged();

        m_parentDirectoryAvailable = false;
        emit parentDirectoryAvailableChanged();

        m_downloadPossible = false;
        emit downloadPossibleChanged();

        m_connectPossible = true;
        emit connectPossibleChanged();

        m_connectLabel = Common::qstr("Conectar");
        emit connectLabelChanged();

        setInfoStatusLabel("Não conectado.");
        return;
    }
    if (!m_networkSession || !m_networkSession->isOpen())
    {
        if (m_manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired)
        {
            if (!m_networkSession) {
                QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
                settings.beginGroup(QLatin1String("QtNetwork"));
                const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
                settings.endGroup();

                QNetworkConfiguration config = m_manager.configurationFromIdentifier(id);
                if ((config.state() & QNetworkConfiguration::Discovered) != QNetworkConfiguration::Discovered) {
                    config = m_manager.defaultConfiguration();
                }

                m_networkSession = new QNetworkSession(config, this);
                connect(m_networkSession, SIGNAL(opened()), this, SLOT(connectToFtp()));
                connect(m_networkSession, SIGNAL(error(QNetworkSession::SessionError)),
                        this, SLOT(enableConnectButton()));
            }
            m_connectPossible = false;
            emit connectPossibleChanged();

            setInfoStatusLabel("正在打开网络会话 ...");
            m_networkSession->open();
            return;
        }
    }
    connectToFtp();
}

void FtpDownloader::setHostname(const QString &url)
{
    m_hostname = url;
}

QString FtpDownloader::hostname() const
{
    return m_hostname;
}

void FtpDownloader::setUsername(const QString &username)
{
    m_username = username;
}

QString FtpDownloader::username() const
{
    return m_username;
}

void FtpDownloader::setPassword(const QString &password)
{
    m_password = password;
}

QString FtpDownloader::password() const
{
    return m_password;
}

QString FtpDownloader::statusLabel() const
{
    return m_statusLabel;
}

void FtpDownloader::setStatusLabel(const QString &status)
{
    m_statusLabel = status;
    emit statusLabelChanged();
}

bool FtpDownloader::parentDirectoryAvailable() const
{
    return m_parentDirectoryAvailable;
}

bool FtpDownloader::downloadPossible() const
{
    return m_downloadPossible;
}

bool FtpDownloader::connectPossible() const
{
    return m_connectPossible;
}

QString FtpDownloader::connectLabel() const
{
    return m_connectLabel;
}

bool FtpDownloader::selectionPossible() const
{
    return m_selectionPossible;
}

bool FtpDownloader::waitIndicatorPossible() const
{
    return m_waitIndicatorPossible;
}

void FtpDownloader::setInfoStatusLabel(const QString &info, bool qstr)
{
    if (qstr) {
        m_statusLabel = Common::qstr("信息：" + info);
    }
    else
    {
        m_statusLabel = Common::qstr("信息：") + info;
    }
    emit statusLabelChanged();
}

void FtpDownloader::setErrorStatusLabel(const QString &error, bool qstr)
{
    if (qstr) {
        m_statusLabel = Common::qstr("错误：" + error);
    }
    else
    {
        m_statusLabel = Common::qstr("错误：") + error;
    }
    emit statusLabelChanged();
}

void FtpDownloader::connectToFtp()
{
    m_waitIndicatorPossible = true;
    emit waitIndicatorPossibleChanged();

    m_ftp = new QFtp(this);
    connect(m_ftp, SIGNAL(commandFinished(int,bool)),
            this, SLOT(ftpCommandFinished(int,bool)));
    connect(m_ftp, SIGNAL(listInfo(QUrlInfo)),
            this, SLOT(addToList(QUrlInfo)));
    connect(m_ftp, SIGNAL(dataTransferProgress(qint64,qint64)),
            this, SLOT(updateDataTransferProgress(qint64,qint64)));

    m_model.clear();
    m_currentPath.clear();

    const QUrl url("ftp://" + m_hostname);
    if (!url.isValid() || url.scheme().toLower() != QLatin1String("ftp")) {
        m_ftp->connectToHost(m_hostname, 21);
        m_ftp->login();
    } else {
        m_ftp->connectToHost(url.host(), url.port(21));
        m_ftp->login(m_username, m_password);
        if (!url.path().isEmpty()) {
            m_currentPath = url.path();
            if (m_currentPath.endsWith('/')) {
                m_currentPath.chop(1);
            }
            m_ftp->cd(m_currentPath);
        }
    }
    m_selectionPossible = true;
    emit selectionPossibleChanged();

    m_connectPossible = false;
    emit connectPossibleChanged();

    m_connectLabel = Common::qstr("Parar");
    emit connectLabelChanged();

    setInfoStatusLabel(QString("Conectando-se com %1 ...").arg(m_hostname));
}

void FtpDownloader::downloadFile()
{
    const QVariant data = m_model.data(m_currentIndexPath);
    if (!data.isValid()) {
        enableDownloadButton();
        return;
    }
    const FtpItem currentItem = data.value<FtpItem>();
    m_fileName = Common::cstr(currentItem.fileName);
    const QString diskFileName = "/accounts/1000/shared/downloads/" + currentItem.fileName;

    if (QFile::exists(diskFileName)) {
        setInfoStatusLabel(diskFileName + Common::qstr(" já existe."), false);
        return;
    }

    m_file = new QFile(diskFileName);
    if (!m_file->open(QIODevice::WriteOnly)) {
        setErrorStatusLabel(m_file->errorString());
        delete m_file;
        m_file = 0;
        return;
    }

    m_ftp->get(m_fileName, m_file);
    m_downloadPossible = false;
    emit downloadPossibleChanged();
    m_progressDialogController.setLabelText(Common::qstr("Transferindo %1 ...").arg(currentItem.fileName));
    m_progressDialogController.show();
}

// 上传文件
void FtpDownloader::uploadFile(const QString &filePath)
{
    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::ReadOnly))
    {
        setErrorStatusLabel(m_file->errorString());
        delete m_file;
        m_file = 0;
        return;
    }
    m_fileName = filePath.mid(filePath.lastIndexOf("/") + 1);
    QString ftpFilePath = m_currentPath + "/" + Common::cstr(m_fileName);

    m_ftp->put(m_file, ftpFilePath);
    m_progressDialogController.setLabelText(Common::qstr("Enviando %1 ...").arg(m_fileName));
    m_progressDialogController.show();
}

void FtpDownloader::cancelDownload()
{
    const int currentCommand = m_ftp->currentCommand();
    connectOrDisconnect();
    if (currentCommand == QFtp::Put)
    {
        m_file->close();
    }
    else if (currentCommand == QFtp::Get) {
        if (m_file->exists())
        {
            m_file->close();
            m_file->remove();
        }
    }
    delete m_file;
    m_file = 0;
    m_progressDialogController.hide();
}

void FtpDownloader::ftpCommandFinished(int, bool error)
{
    if (m_ftp->currentCommand() == QFtp::ConnectToHost) {
        if (error) {
            setErrorStatusLabel(QString("Impossível conectar-se com %1, verifique o endereço.").arg(m_hostname));
            connectOrDisconnect();

            m_waitIndicatorPossible = false;
            emit waitIndicatorPossibleChanged();
            return;
        }
        Common::setAppSetting(hostnameSetting, m_hostname);
        Common::setAppSetting(usernameSetting, m_username);
        Common::setAppSetting(passwordSetting, m_password);

        m_statusLabel = Common::qstr("/");
        emit statusLabelChanged();

        m_connectPossible = true;
        emit connectPossibleChanged();
        return;
    }
    else if (m_ftp->currentCommand() == QFtp::Login) {
        m_ftp->list();
    }
    else if (m_ftp->currentCommand() == QFtp::Get) {
        if (error) {
            setErrorStatusLabel(QString("%1 falhou.").arg(m_fileName));
            if (m_file) {
                m_file->close();
                m_file->remove();
            }
        } else {
            if (m_file) {
                const QFileInfo actualDir(*m_file);
                setInfoStatusLabel(QString("%1/%2 transferido com êxito.").arg(actualDir.absolutePath()).arg(m_fileName));
                m_file->close();
            } else {
                setErrorStatusLabel(QString("%1 falhou.").arg(m_fileName));
            }
        }
        delete m_file;
        m_file = 0;

        enableDownloadButton();
        m_progressDialogController.hide();
    }
    else if (m_ftp->currentCommand() == QFtp::Put) {
        if (error) {
            setErrorStatusLabel(QString("Falha no envio."));
        } else {
            setInfoStatusLabel(QString("Êxito no envio."));
            refresh();
        }
        m_file->close();
        delete m_file;
        m_file = 0;

        m_progressDialogController.hide();
    }
    else if (m_ftp->currentCommand() == QFtp::List) {
        if (m_model.isEmpty()) {
            FtpItem item;
            item.fileName = tr("<empty>");
            m_model.append(item);
            m_selectionPossible = false;
            emit selectionPossibleChanged();
        }
        if (!m_currentPath.isEmpty()) {
            m_parentDirectoryAvailable = true;
            emit parentDirectoryAvailableChanged();
        }
        m_waitIndicatorPossible = false;
        emit waitIndicatorPossibleChanged();
    }
}

void FtpDownloader::addToList(const QUrlInfo &urlInfo)
{
    if (urlInfo.isDir() && (urlInfo.name() == ".")) {
        return;
    }

    FtpItem item;
    item.fileName = Common::qstr(urlInfo.name());
    item.fileSize = urlInfo.size();
    item.owner = urlInfo.owner();
    item.group = urlInfo.group();
    item.time = urlInfo.lastModified();
    item.isDirectory = urlInfo.isDir();

    const bool wasEmpty = m_model.isEmpty();
    m_model.append(item);

    if (wasEmpty) {
        m_selectionPossible = true;
        emit selectionPossibleChanged();
    }
}

void FtpDownloader::processItem(const QVariantList &indexPath)
{
    if (!m_selectionPossible) {
        return;
    }

    m_currentIndexPath = indexPath;
    enableDownloadButton();

    if (!indexPath.isEmpty()) {
        const FtpItem item = m_model.data(indexPath).value<FtpItem>();
        if (item.isDirectory) {
            if (item.fileName == "..") {
                cdToParent();
                return;
            }
            m_waitIndicatorPossible = true;
            emit waitIndicatorPossibleChanged();

            m_model.clear();

            m_currentPath += '/';
            m_currentPath += Common::cstr(item.fileName);

            m_ftp->cd(Common::cstr(item.fileName));
            m_ftp->list();

            m_statusLabel = Common::qstr(m_currentPath);
            emit statusLabelChanged();

            m_parentDirectoryAvailable = true;
            emit parentDirectoryAvailableChanged();
            return;
        }
    }
}

bool FtpDownloader::isDirectory(const QVariantList &indexPath)
{
    const FtpItem item = m_model.data(indexPath).value<FtpItem>();
    return item.isDirectory;
}

void FtpDownloader::cdToParent()
{
    m_waitIndicatorPossible = true;
    emit waitIndicatorPossibleChanged();

    m_model.clear();

    m_currentIndexPath.clear();
    enableDownloadButton();

    m_currentPath = m_currentPath.left(m_currentPath.lastIndexOf('/'));
    if (m_currentPath.isEmpty()) {
        m_parentDirectoryAvailable = false;
        emit parentDirectoryAvailableChanged();
        m_ftp->cd("/");
        m_statusLabel = Common::qstr("/");
        emit statusLabelChanged();
    } else {
        m_ftp->cd(m_currentPath);
        m_statusLabel = Common::qstr(m_currentPath);
        emit statusLabelChanged();
    }
    m_ftp->list();
}

void FtpDownloader::cdToRoot()
{
    m_waitIndicatorPossible = true;
    emit waitIndicatorPossibleChanged();

    m_model.clear();
    m_currentIndexPath.clear();
    enableDownloadButton();

    m_currentPath = "";
    m_ftp->cd("/");
    m_ftp->list();

    m_statusLabel = Common::qstr("/");
    emit statusLabelChanged();

    m_parentDirectoryAvailable = false;
    emit parentDirectoryAvailableChanged();
}

void FtpDownloader::refresh()
{
    m_waitIndicatorPossible = true;
    emit waitIndicatorPossibleChanged();

    m_model.clear();
    m_currentIndexPath.clear();
    enableDownloadButton();

    m_ftp->cd(m_currentPath + "/");
    m_ftp->list();
}

void FtpDownloader::updateDataTransferProgress(qint64 readBytes, qint64 totalBytes)
{
    const float progress = readBytes / (float) totalBytes * 100.0f;
    m_progressDialogController.setProgress(progress);
}

void FtpDownloader::enableDownloadButton()
{
    const QVariant itemData = m_model.data(m_currentIndexPath);
    if (itemData.isValid()) {
        const FtpItem item = itemData.value<FtpItem>();
        m_downloadPossible = !item.isDirectory;
    } else {
        m_downloadPossible = false;
    }
    emit downloadPossibleChanged();
}

void FtpDownloader::enableConnectButton()
{
    QNetworkConfiguration config = m_networkSession->configuration();
    QString id;
    if (config.type() == QNetworkConfiguration::UserChoice) {
        id = m_networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
    }
    else {
        id = config.identifier();
    }

    QSettings settings(QSettings::UserScope, QLatin1String("Trolltech"));
    settings.beginGroup(QLatin1String("QtNetwork"));
    settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
    settings.endGroup();

    m_connectPossible = true;
    emit connectPossibleChanged();

    setInfoStatusLabel("Não conectado.");
}

bb::cascades::QListDataModel<FtpItem> *FtpDownloader::model()
{
    return &m_model;
}

MessageBoxController *FtpDownloader::messageBoxController()
{
    return &m_messageBoxController;
}

ProgressDialogController *FtpDownloader::progressDialogController()
{
    return &m_progressDialogController;
}
