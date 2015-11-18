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

#ifndef SCXMLPARSER_H
#define SCXMLPARSER_H

#include <qscxmlerror.h>

#include <QStringList>
#include <QString>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
class QScxmlStateMachine;

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
    void instantiateDataModel(QScxmlStateMachine *stateMachine) const;

    State state() const;
    QVector<QScxmlError> errors() const;
    void addError(const QString &msg);

private:
    friend class QScxmlParserPrivate;
    QScxmlParserPrivate *p;
};

QT_END_NAMESPACE

#endif // SCXMLPARSER_H
