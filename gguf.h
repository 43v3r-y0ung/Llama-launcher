#ifndef GGUF_H
#define GGUF_H

#include <QString>
#include <QtGlobal>

// Minimal, read-only view of the metadata stored in a GGUF model file header.
struct GgufInfo
{
    bool valid = false;
    QString error;

    QString architecture;   // general.architecture, e.g. "qwen2"
    QString name;           // general.name
    QString sizeLabel;      // general.size_label, e.g. "7B"
    quint64 contextLength = 0;   // <arch>.context_length
    quint64 blockCount = 0;      // <arch>.block_count (transformer layers)
    quint64 expertCount = 0;     // <arch>.expert_count (0 = dense model)
    quint64 expertUsedCount = 0; // <arch>.expert_used_count

    qint64 totalSize = 0;   // bytes, summed over all parts of a split model
    int splitCount = 1;

    bool isMoe() const { return expertCount > 1; }
};

// Reads only the key/value header of the file (memory-mapped), so it is fast even for large models.
GgufInfo readGgufInfo(const QString &path);

// True for files that should not be offered as a model on their own:
// multimodal projectors and the non-first parts of split models.
bool isAuxiliaryGgufFile(const QString &fileName);

QString formatBytes(qint64 bytes);

#endif // GGUF_H
