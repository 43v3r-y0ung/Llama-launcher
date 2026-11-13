#pragma once

#include <QCoreApplication>
#include <QSet>
#include <QString>
#include <QStringList>

// Turns known llama-server warnings into short, actionable hints for the Log tab.
// Each hint is reported once per server run.
class LogAdvisor
{
    Q_DECLARE_TR_FUNCTIONS(LogAdvisor)

public:
    void reset();

    // Returns true when the line produced a hint that was not reported yet in this run.
    bool feedLine(const QString &line);

    const QStringList &hints() const { return m_hints; }

private:
    void add(const QString &id, const QString &text);

    QSet<QString> m_seen;
    QStringList m_hints;
};
