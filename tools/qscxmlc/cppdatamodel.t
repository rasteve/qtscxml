QString ${datamodel}::evaluateToString(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    *ok = true;
    switch (id) {
${evaluateToStringCases}    default:
        Q_UNREACHABLE();
        *ok = false;
        return QString();
    }
}

bool ${datamodel}::evaluateToBool(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    *ok = true;
    switch (id) {
${evaluateToBoolCases}    default:
        Q_UNREACHABLE();
        *ok = false;
        return false;
    }
}

QVariant ${datamodel}::evaluateToVariant(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    *ok = true;
    switch (id) {
${evaluateToVariantCases}    default:
        Q_UNREACHABLE();
        *ok = false;
        return QVariant();
    }
}

void ${datamodel}::evaluateToVoid(QScxmlExecutableContent::EvaluatorId id, bool *ok)
{
    *ok = true;
    switch (id) {
${evaluateToVoidCases}    default:
        Q_UNREACHABLE();
        *ok = false;
    }
}
