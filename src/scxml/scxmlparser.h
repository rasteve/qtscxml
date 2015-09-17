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

#ifndef SCXMLPARSER_H
#define SCXMLPARSER_H

#include <QtScxml/scxmlstatemachine.h>

#include <QStringList>
#include <QString>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;

class QScxmlParserPrivate;
class Q_SCXML_EXPORT QScxmlParser
{
public:
    class Q_SCXML_EXPORT Loader
    {
    public:
        Loader(QScxmlParser *parser);
        virtual ~Loader();
        virtual QByteArray load(const QString &name, const QString &baseDir, bool *ok) = 0;

    protected:
        QScxmlParser *parser() const;

    private:
        QScxmlParser *m_parser;
    };

    enum State {
        StartingParsing,
        ParsingScxml,
        ParsingError,
        FinishedParsing,
    };

public:
    QScxmlParser(QXmlStreamReader *xmlReader);
    ~QScxmlParser();

    QString fileName() const;
    void setFileName(const QString &fileName);

    Loader *loader() const;
    void setLoader(Loader *newLoader);

    void parse();
    QScxmlStateMachine *instantiateStateMachine() const;
    void instantiateDataModel(QScxmlStateMachine *table) const;

    State state() const;
    QVector<QScxmlError> errors() const;
    void addError(const QString &msg);

private:
    friend QScxmlParserPrivate;
    QScxmlParserPrivate *p;
};

QT_END_NAMESPACE

#endif // SCXMLPARSER_H
