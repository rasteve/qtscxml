/****************************************************************************
 **
 ** Copyright (c) 2014 Digia Plc
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

#include "scxmlcppdumper.h"
#include <algorithm>

namespace Scxml {

const char *headerStart =
        "#include <qscxml/scxmlstatetable.h>\n"
        "\n";

void CppDumper::dump(StateTable *table)
{
    this->table = table;
   QString className = options.basename;
   if (!className.isEmpty())
       className.append(QLatin1Char('_'));
   className.append(table->_name);
   if (!className.isEmpty())
       className.append(QLatin1Char('_'));
   className.append(l("StateMachine"));
    s << l(headerStart);
    if (!options.namespaceName.isEmpty())
        s << l("namespace ") << options.namespaceName << l(" {\n");
    s << l("class ") << className << l(" : public Scxml::StateTable {\n");
    s << "Q_OBJECT\n";
    dumpDeclareStates();
    dumpDeclareSignalsForEvents();
    s << "slots:\n";
    dumpExecutableContent();
    s << "public:\n";
    dumpInit();
    s << l("};\n");
    if (!options.namespaceName.isEmpty())
        s << l("} // namespace ") << options.namespaceName << l("\n");
}


void CppDumper::dumpDeclareStates()
{
    loopOnSubStates(table, [this](QState *state) -> bool {
        s << l("Scxml::ScxmlState *state_") << table->objectId(state, false) << l(";\n");
        return true;
    }, Q_NULLPTR, [this](QAbstractState *state) -> void {
        s << b(state->metaObject()->className()) << l(" *state_") << table->objectId(state, false)
          << l(";\n");
    });
}

void CppDumper::dumpDeclareSignalsForEvents()
{
    QSet<QByteArray> knownEvents;
    loopOnSubStates(table, [&knownEvents](QState *state) -> bool {
        foreach (QAbstractTransition *t, state->transitions()) {
            if (ScxmlTransition *scxmlT = qobject_cast<ScxmlTransition *>(t)) {
                foreach (const QByteArray &event, scxmlT->eventSelector)
                    knownEvents.insert(event);
            }
        }
        return true;
    }, Q_NULLPTR, Q_NULLPTR);
    QList<QByteArray> knownEventsList = knownEvents.toList();
    std::sort(knownEventsList.begin(), knownEventsList.end());
    bool hasSignals = false;
    foreach (QByteArray event, knownEventsList) {
        if (event.startsWith(b("done.")) || event.startsWith(b("qsignal."))
                || event.startsWith(b("qevent.")))
            continue;
        if (!hasSignals)
            s << l("signals:\n");
        hasSignals = true;
        s << l("void event_") << event.replace('.', '_') << "();\n";
    }
    if (hasSignals)
        s << l("public:\n");
}

void CppDumper::dumpExecutableContent()
{

}

void CppDumper::dumpInit()
{
    s << l("    bool init(QJSEngine *engine) Q_DECL_OVERRIDE {\n");
    loopOnSubStates(table, [this](QState *state) -> bool {
        QString stateName = QString::fromUtf8(table->objectId(state, false));
        s << l("        state_") << stateName << l(" = new Scxml::ScxmlState(");
        if (state->parentState())
            s << table->objectId(state->parentState(), false);
        s << l(");\n");
        s << l("        addId(QString::fromUtf8(\"") << stateName << l("\", state_") << stateName
          << l(");\n");
        return true;
    }, Q_NULLPTR, [this](QAbstractState *state) -> void {
        QString stateName = QString::fromUtf8(table->objectId(state, false));
        s << l("        state_") << stateName << l(" = new ")
          << b(state->metaObject()->className()) << l("(");
        if (state->parentState())
            s << table->objectId(state->parentState(), false);
        s << l(");\n");
        s << l("        addId(QString::fromUtf8(\"") << stateName << l("\", state_") << stateName
          << l(");\n");
    });
    loopOnSubStates(table, [this](QState *state) -> bool {
        foreach (QAbstractTransition *t, state->transitions()) {
            if (Scxml::ScxmlTransition *scTransition = qobject_cast<Scxml::ScxmlTransition *>(t)) {
                s << scTransition->eventSelector.join(' ');
                cEscape(scTransition->conditionalExp);
            }
        }
        return true;
    }, Q_NULLPTR, Q_NULLPTR);
    s << l("    }\n");
}

QString CppDumper::cEscape(const QString &str)
{
    QString res = str;
    return res.replace(QLatin1Char('\\'), QLatin1String("\\\\"))
            .replace(QLatin1Char('\"'), QLatin1String("\\\""));
}

} // namespace Scxml
