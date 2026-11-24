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

#ifndef LOCALESECTIONMAPS_P_H
#define LOCALESECTIONMAPS_P_H

#include "languagemodel.h"

#include <QHash>

namespace {
static const inline QHash<const QString, LocaleSection::Section> variableTolocaleSectionMap =
{
    { QStringLiteral("LC_ALL"),      LocaleSection::Section::All },
    { QStringLiteral("LC_CTYPE"),    LocaleSection::Section::CType },
    { QStringLiteral("LC_COLLATE"),  LocaleSection::Section::Collate },
    { QStringLiteral("LC_MESSAGES"), LocaleSection::Section::Messages },
    { QStringLiteral("LC_MONERARY"), LocaleSection::Section::Monetary },
    { QStringLiteral("LC_NUMERIC"),  LocaleSection::Section::Numeric },
    { QStringLiteral("LC_TIME"),     LocaleSection::Section::Time },
#ifdef __GLIBC__
    { QStringLiteral("LC_ADDRESS"),        LocaleSection::Section::Address },
    { QStringLiteral("LC_IDENTIFICATION"), LocaleSection::Section::Identification },
    { QStringLiteral("LC_MEASUREMENT"),    LocaleSection::Section::Measurement },
    { QStringLiteral("LC_NAME"),           LocaleSection::Section::Name },
    { QStringLiteral("LC_PAPER"),          LocaleSection::Section::Paper },
    { QStringLiteral("LC_TELEPHONE"),      LocaleSection::Section::Telephone },
#endif // __GLIBC__

};

//static const QHash<const LocaleSection::Section, const QString> localeSectionToNameMap =
static const inline QHash<const LocaleSection::Section, QString> localeSectionToNameMap =
{
        { LocaleSection::Section::All,            QStringLiteral("All") },
        { LocaleSection::Section::CType,          QStringLiteral("Character Conversions") },
        { LocaleSection::Section::Collate,        QStringLiteral("Sorting and Collation") },
        { LocaleSection::Section::Messages,       QStringLiteral("Messages") },
        { LocaleSection::Section::Monetary,       QStringLiteral("Currency") },
        { LocaleSection::Section::Numeric,        QStringLiteral("Numbers") },
        { LocaleSection::Section::Time,           QStringLiteral("Time and Date") },
#ifdef __GLIBC__
        { LocaleSection::Section::Address,        QStringLiteral("Addresses") },
        { LocaleSection::Section::Identification, QStringLiteral("Locale Metadata") },
        { LocaleSection::Section::Measurement,    QStringLiteral("Measurement System") },
        { LocaleSection::Section::Name,           QStringLiteral("Personal Names") },
        { LocaleSection::Section::Paper,          QStringLiteral("Paper Size") },
        { LocaleSection::Section::Telephone,      QStringLiteral("Telephone Numbers") },
#endif // __GLIBC__
};

static const inline QHash<const LocaleSection::Section, const char*> localeSectionToVariableNameMap =
    {
        { LocaleSection::Section::All,            "LC_ALL"},
        { LocaleSection::Section::CType,          "LC_CTYPE"},
        { LocaleSection::Section::Collate,        "LC_COLLATE"},
        { LocaleSection::Section::Messages,       "LC_MESSAGES"},
        { LocaleSection::Section::Monetary,       "LC_MONERARY"},
        { LocaleSection::Section::Numeric,        "LC_NUMERIC"},
        { LocaleSection::Section::Time,           "LC_TIME"},
#ifdef __GLIBC__
        { LocaleSection::Section::Address,        "LC_ADDRESS"},
        { LocaleSection::Section::Identification, "LC_IDENTIFICATION"},
        { LocaleSection::Section::Measurement,    "LC_MEASUREMENT"},
        { LocaleSection::Section::Name,           "LC_NAME"},
        { LocaleSection::Section::Paper,          "LC_PAPER"},
        { LocaleSection::Section::Telephone,      "LC_TELEPHONE" },
#endif // __GLIBC__
};

}
#endif
