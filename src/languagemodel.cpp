/*
 * Copyright (C) 2013 Jolla Ltd. <pekka.vuorela@jollamobile.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include "languagemodel.h"
#include "localeconfig.h"

#include <QDir>
#include <QSaveFile>
#include <QDebug>
#include <QSettings>
#include <QHash>
#include <QProcess>

#include <nemo-dbus/connection.h>
#include <nemo-dbus/interface.h>

namespace {
const char * const LanguageSupportDirectory = "/usr/share/jolla-supported-languages";

bool nameLessThan(const Language &lang1, const Language &lang2)
{
    return (lang1.name().localeAwareCompare(lang2.name()) <= 0);
}

}

Language::Language(QString name, QString localeCode, QString region, QString regionLabel)
    : m_name(name), m_localeCode(localeCode), m_region(region), m_regionLabel(regionLabel)
{
}

QString Language::name() const
{
    return m_name;
}

QString Language::localeCode() const
{
    return m_localeCode;
}

QString Language::region() const
{
    return m_region;
}

QString Language::regionLabel() const
{
    return m_regionLabel;
}

LanguageModel::LanguageModel(QObject *parent)
    : QAbstractListModel(parent),
      m_currentIndex(-1)
{
    m_languages = supportedLanguages();
    readCurrentLocale();
}

LanguageModel::~LanguageModel()
{
}

QHash<int, QByteArray> LanguageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[LocaleRole] = "locale";
    roles[RegionRole] = "region";
    roles[RegionLabelRole] = "regionLabel";

    return roles;
}

void LanguageModel::readCurrentLocale()
{
    QFile localeConfig;
    QList<QString> configsPaths{localeConfigPath(), preferredLocaleConfigPath()};

    foreach (const QString &configPath, configsPaths) {
        localeConfig.setFileName(configPath);
        if (localeConfig.exists() && localeConfig.open(QIODevice::ReadOnly)) {
            break;
        }
    }

    if (!localeConfig.isOpen()) {
        return;
    }

    QString locale;
    while (!localeConfig.atEnd()) {
        QString line = localeConfig.readLine().trimmed();
        if (line.startsWith("LANG=")) {
             locale = line.mid(5);
             break;
        }
    }

    m_currentIndex = getLocaleIndex(locale);
}



int LanguageModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent)
    return m_languages.count();
}

QVariant LanguageModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= m_languages.count()) {
        return QVariant();
    }

    const Language &language = m_languages.at(row);
    switch (role) {
    case NameRole:
        return language.name();
    case LocaleRole:
        return language.localeCode();
    case RegionRole:
        return language.region();
    case RegionLabelRole:
        return language.regionLabel();
    default:
        return QVariant();
    }
}

int LanguageModel::currentIndex() const
{
    return m_currentIndex;
}

QString LanguageModel::languageName(int index) const
{
    if (index < 0 || index >= m_languages.count()) {
        return QString();
    }
    return m_languages.at(index).name();
}

QString LanguageModel::locale(int index) const
{
    if (index < 0 || index >= m_languages.count()) {
        return QString();
    }
    return m_languages.at(index).localeCode();
}

void LanguageModel::setSystemLocale(const QString &localeCode, LocaleUpdateMode updateMode)
{
    int ret = QProcess::execute(QLatin1String("/usr/libexec/setlocale"), QStringList(localeCode));
    if (ret != 0) {
        qWarning() << "Setting user locale failed!";
        return;
    }

    int oldLocale = m_currentIndex;
    m_currentIndex = getLocaleIndex(localeCode);
    if (m_currentIndex != oldLocale) {
        emit currentIndexChanged();
    }

    if (updateMode == UpdateAndReboot) {
        NemoDBus::Interface dsmeInterface(
                this, QDBusConnection::systemBus(),
                "com.nokia.dsme", "/com/nokia/dsme/request", "com.nokia.dsme.request");
        dsmeInterface.blockingCall("req_reboot");
    }
}

QList<Language> LanguageModel::supportedLanguages()
{
    // get supported languages
    QDir languageDirectory(LanguageSupportDirectory);
    QFileInfoList fileInfoList = languageDirectory.entryInfoList(QStringList("*.conf"), QDir::Files);
    QList<Language> languages;

    foreach (const QFileInfo &fileInfo, fileInfoList) {
        QSettings settings(fileInfo.filePath(), QSettings::IniFormat);
        settings.setIniCodec("UTF-8");
        QString name = settings.value("Name").toString();
        QString localeCode = settings.value("LocaleCode").toString();
        QString region = settings.value("Region").toString();
        //% "Region: %1"
        QString regionLabel = settings.value("RegionLabel", qtTrId("systemsettings-la-region")).toString();
        if (name.isEmpty() || localeCode.isEmpty()) {
            continue;
        }
        Language newLanguage(name, localeCode, region, regionLabel);
        languages.append(newLanguage);
    }

    std::sort(languages.begin(), languages.end(), nameLessThan);
    return languages;
}

int LanguageModel::getLocaleIndex(const QString &locale) const
{
    int i = 0;
    foreach (Language language, m_languages) {
        if (language.localeCode() == locale) {
            return i;
        }
        i++;
    }

    return -1;
}

void LanguageModel::setAdditionalLocaleSection(LocaleSection section, const QString &localeCode, bool remove)
{
    QFile localeConfig;
    localeConfig.setFileName(localeConfigPath());

    QSaveFile localeConfigNew(localeConfig.fileName());

    const char* variable = LocaleSection::variableName(section.section());
    const QByteArray replacement = (QStringLiteral("%1=\"%2\"").arg(variable).arg(localeCode)).toLocal8Bit();

    QList<QByteArray> content({});

    if (localeConfig.exists()) {
        localeConfig.open(QIODevice::ReadOnly);

        if (!localeConfig.isOpen()) {
            return;
        }
        content = localeConfig.readAll().split('\n');
        localeConfig.close();
    }

    content.removeAt(content.indexOf(replacement));
    if (!remove) {
        content.append(replacement);
    }

    if (!QDir().exists(QFileInfo(localeConfigNew.fileName()).absolutePath())) {
        int ret = QProcess::execute(QLatin1String("/usr/libexec/setlocale"), QStringList("-createDir"));
        if (ret != 0) {
            qWarning() << "Creating user locale dir failed!";
            return;
        }
    }

    localeConfigNew.open(QIODevice::WriteOnly);

    if (!localeConfigNew.isOpen()) {
        qWarning() << "Could not open file!";
        return;
    }

    QTextStream stream(&localeConfigNew);
    stream << content.join('\n');
    stream.flush();

    if (!localeConfigNew.commit())
        qWarning() << "Writing file failed!";

}

QString LocaleSection::name() const
{
    switch(m_section) {
        case Section::All:
            return QStringLiteral("All");
        case Section::CType:
            return QStringLiteral("Character Conversions");
        case Section::Collate:
            return QStringLiteral("Sorting and Collation");
        case Section::Messages:
            return QStringLiteral("Messages");
        case Section::Monetary:
            return QStringLiteral("Currency");
        case Section::Numeric:
            return QStringLiteral("Numbers");
        case Section::Time:
            return QStringLiteral("Time and Date");
#ifdef __GLIBC__
        case Section::Address:
            return QStringLiteral("Addresses");
        case Section::Identification:
            return QStringLiteral("Locale Metadata");
        case Section::Measurement:
            return QStringLiteral("Measurement System");
        case Section::Name:
            return QStringLiteral("Personal Names");
        case Section::Paper:
            return QStringLiteral("Paper Size");
        case Section::Telephone:
            return QStringLiteral("Telephone Numbers");
#endif // __GLIBC__
        default:
            return QString();
    }
}

const char* LocaleSection::variableName(const LocaleSection::Section &section)
{
    switch(section) {
        case Section::All:
            return "LC_ALL";
        case Section::CType:
            return "LC_CTYPE";
        case Section::Collate:
            return "LC_COLLATE";
        case Section::Messages:
            return "LC_MESSAGES";
        case Section::Monetary:
            return "LC_MONERARY";
        case Section::Numeric:
            return "LC_NUMERIC";
        case Section::Time:
            return "LC_TIME";
#ifdef __GLIBC__
        case Section::Address:
            return "LC_ADDRESS";
        case Section::Identification:
            return "LC_IDENTIFICATION";
        case Section::Measurement:
            return "LC_MEASUREMENT";
        case Section::Name:
            return "LC_NAME";
        case Section::Paper:
            return "LC_PAPER";
        case Section::Telephone:
            return "LC_TELEPHONE";
#endif // __GLIBC__
        default:
            return "";
    }
}
