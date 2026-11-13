#pragma once

#include <QCoreApplication>
#include <QList>
#include <QString>

// Turns known llama-server warnings into short, actionable hints for the Log tab.
// Each hint is reported once per server run.
class LogAdvisor
{
    Q_DECLARE_TR_FUNCTIONS(LogAdvisor)

public:
    enum class HintId {
        FitSkipped,    // --fit gave up because -ngl was set by hand
        FlashAttnOff,  // the server turned Flash Attention off by itself
        ContextShift,  // the model does not support context shifting
        Mmap,          // CPU tensor overrides with a memory-mapped model file
        OutOfMemory,
    };

    struct Hint {
        HintId id;
        QString text;
    };

    void reset();

    // Returns true when the line produced a hint that was not reported yet in this run.
    bool feedLine(const QString &line);

    const QList<Hint> &hints() const { return m_hints; }

private:
    void add(HintId id, const QString &text);

    quint32 m_seen = 0; // bit per HintId
    QList<Hint> m_hints;
};
