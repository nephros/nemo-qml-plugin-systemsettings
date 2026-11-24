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
#include "localesectionmaps_p.h"

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
    : QAbstractListModel(parent)
    , m_currentIndex(-1)
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

bool LocaleSection::isValid() const
{
    return ( (m_section != Section::Invalid)
          && (m_section < Section::NumSections)
          && !name().isNull()
          );
};
void LocaleSectionModel::setLocaleSection(const QString& variable, const QString &localeCode, UpdateMode updateMode)
{
    QFile localeConfig;
    localeConfig.setFileName(localeConfigPath());

    QSaveFile localeConfigNew(localeConfig.fileName());

    QList<QByteArray> content({});

    if (localeConfig.exists()) {
        localeConfig.open(QIODevice::ReadOnly);

        if (!localeConfig.isOpen()) {
            return;
        }
        content = localeConfig.readAll().split('\n');
        localeConfig.close();
    }

    // remove old, maybe add new:
    const QByteArray newValue = QByteArray(variable.toUtf8()) + QByteArrayLiteral("=") + localeCode.toUtf8();
    for (QList<QByteArray>::iterator it = content.begin(); it != content.end(); ++it) {
        if (!it->startsWith(variable.toUtf8())) {
            continue;
        } else {
            if (updateMode == UpdateMode::RemoveValue)
                content.erase(it);
            else
                *it = newValue;
            break;
        }
    }

    // ensure the location exists:
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

QList<LocaleSection> LocaleSectionModel::getLocaleSections()
{

    QFile localeConfig;
    localeConfig.setFileName(localeConfigPath());

    QList<LocaleSection> result;

    for (int i = 0; i < LocaleSection::Section::NumSections; i++) {
        const LocaleSection lc((LocaleSection::Section) i);
        result.append(lc);
        qDebug() << "Added:" << lc.name();
    }

    if (localeConfig.exists()) {
        localeConfig.open(QIODevice::ReadOnly);

        if (!localeConfig.isOpen()) {
            return result;
        }

        QList<QPair<QString, QString>> found;
        while (!localeConfig.atEnd()) {
            const QString line = localeConfig.readLine().trimmed();
            const QPair<QString, QString> elem( { line.split('=').first(), line.split('=').last() } );
            if(elem.first.startsWith(QStringLiteral("LC_")))
               found.append(elem);
        }

        localeConfig.close();

        if (found.count() > 0) {
           for (QPair<QString, QString> f : found) {
               for (QList<LocaleSection>::iterator it = result.begin(); it != result.end(); ++it) {
                   if(it->variableName() == f.first) {
                       it->setValue(f.second);
                       break;
                   }
               }
           }
        }

    }

    return result;
}

LocaleSection::LocaleSection(Section section, QString value)
        : m_section(section)
        , m_value(value)
{
}

LocaleSection LocaleSection::fromVariableName(const QString& var)
{
    return variableTolocaleSectionMap.value(var);
}

const char* LocaleSection::variableName() const
{
    return localeSectionToVariableNameMap.value(m_section);
}

QString LocaleSection::name() const
{
    if (isValid())
        return localeSectionToNameMap.value(m_section);
    return QString();
}

/*
LocaleSection::Section LocaleSection::section() const
{
    return m_section;
}
*/

LocaleSectionModel::LocaleSectionModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_model(LanguageModel(this))
{
    m_languages = LanguageModel::supportedLanguages();
    m_languages.insert(0, Language("C", "C", QString(), QStringLiteral("default/none")));
    getLocaleSections();
}

LocaleSectionModel::~LocaleSectionModel()
{
}

QHash<int, QByteArray> LocaleSectionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[VariableRole] = "variable";
    roles[ValueRole] = "value";

    return roles;
}


int LocaleSectionModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent)
    return m_localesections.count();
}


QVariant LocaleSectionModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= m_localesections.count()) {
        return QVariant();
    }

    const LocaleSection &section = m_localesections.at(row);
    switch (role) {
    case NameRole:
        return section.name();
//    case SectionRole:
//        return section.localeCode();
    case VariableRole:
        return section.variableName();
    case ValueRole:
        return section.value();
    default:
        return QVariant();
    }
}

int LocaleSectionModel::currentIndex() const
{
    return m_currentIndex;
}

QString LocaleSectionModel::sectionName(int index) const
{
    if (index < 0 || index >= m_localesections.count()) {
        return QString();
    }
    return m_localesections.at(index).name();
}

QString LocaleSectionModel::variableName(int index) const
{
    if (index < 0 || index >= m_localesections.count()) {
        return QString();
    }
    return m_localesections.at(index).variableName();
}

QString LocaleSectionModel::value(int index) const
{
    if (index < 0 || index >= m_localesections.count()) {
        return QString();
    }
    return m_localesections.at(index).value();
}

