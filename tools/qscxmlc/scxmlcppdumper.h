/******************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtScxml module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

#ifndef CPPDUMPER_H
#define CPPDUMPER_H

#include "qscxmlglobals.h"
#include "qscxmlstatemachine.h"

#include <QtScxml/private/qscxmlparser_p.h>

#include <QTextStream>

QT_BEGIN_NAMESPACE

struct ClassDump;

struct TranslationUnit
{
    TranslationUnit()
        : useCxx11(true)
        , mainDocument(Q_NULLPTR)
    {}

    QString outHFileName, outCppFileName;
    QString namespaceName;
    bool useCxx11;
    DocumentModel::ScxmlDocument *mainDocument;
    QHash<DocumentModel::ScxmlDocument *, QString> classnameForDocument;
    QList<TranslationUnit *> dependencies;

    QList<DocumentModel::ScxmlDocument *> otherDocuments() const
    {
        auto docs = classnameForDocument.keys();
        docs.removeOne(mainDocument);
        return docs;
    }
};

class CppDumper
{
public:
    CppDumper(QTextStream &headerStream, QTextStream &cppStream)
        : h(headerStream)
        , cpp(cppStream)
    {}

    void dump(TranslationUnit *unit);

    static QString mangleId(const QString &id);

private:
    void writeHeaderStart(const QString &headerGuard, const QStringList &forwardDecls);
    void writeClass(const ClassDump &clazz);
    void writeHeaderEnd(const QString &headerGuard, const QStringList &metatypeDecls);
    void writeImplStart(const QVector<ClassDump> &allClazzes);
    void writeImplBody(const ClassDump &clazz);
    void writeImplEnd();

private:
    QTextStream &h;
    QTextStream &cpp;

    static QByteArray b(const char *str) { return QByteArray(str); }
    static QLatin1String l (const char *str) { return QLatin1String(str); }

    TranslationUnit *m_translationUnit;
};

QT_END_NAMESPACE

#endif // CPPDUMPER_H
