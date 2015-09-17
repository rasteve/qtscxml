/****************************************************************************
 **
 ** Copyright (c) 2015 Digia Plc
 ** For any questions to Digia, please use contact form at http://qt.digia.com/
 **
 ** All Rights Reserved.
 **
 ** NOTICE: All information contained herein is, and remains
 ** the property of Digia Plc and its suppliers,
 ** if any. The intellectual and technical concepts contained
 ** herein are proprietary to Digia Plc
 ** and its suppliers and may be covered by Finnish and Foreign Patents,
 ** patents in process, and are protected by trade secret or copyright law.
 ** Dissemination of this information or reproduction of this material
 ** is strictly forbidden unless prior written permission is obtained
 ** from Digia Plc.
 ****************************************************************************/

#ifndef CPPDUMPER_H
#define CPPDUMPER_H

#include "scxmlglobals.h"
#include "scxmlstatemachine.h"

#include <QtScxml/private/scxmlparser_p.h>

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
    void writeHeaderStart(const QString &headerGuard);
    void writeClass(const ClassDump &clazz);
    void writeHeaderEnd(const QString &headerGuard);
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
