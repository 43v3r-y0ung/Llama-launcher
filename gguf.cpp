#include "gguf.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QVariant>
#include <cstring>

namespace {

enum GgufType : quint32 {
    U8 = 0, I8 = 1, U16 = 2, I16 = 3, U32 = 4, I32 = 5, F32 = 6, BOOL = 7,
    STRING = 8, ARRAY = 9, U64 = 10, I64 = 11, F64 = 12,
};

int scalarSize(quint32 type)
{
    switch (type) {
    case U8: case I8: case BOOL: return 1;
    case U16: case I16: return 2;
    case U32: case I32: case F32: return 4;
    case U64: case I64: case F64: return 8;
    default: return -1;
    }
}

// Bounds-checked little-endian reader over the mapped header bytes.
class Reader
{
public:
    Reader(const uchar *data, qint64 size) : m_data(data), m_size(size) {}

    bool ok() const { return m_ok; }

    template <typename T> T read()
    {
        T v{};
        if (!need(sizeof(T)))
            return v;
        std::memcpy(&v, m_data + m_pos, sizeof(T)); // GGUF is little-endian, as is x86/ARM Linux
        m_pos += sizeof(T);
        return v;
    }

    QString readString()
    {
        const quint64 len = read<quint64>();
        if (!need(len))
            return {};
        const QString s = QString::fromUtf8(reinterpret_cast<const char *>(m_data + m_pos), qsizetype(len));
        m_pos += qint64(len);
        return s;
    }

    void skip(quint64 n)
    {
        if (need(n))
            m_pos += qint64(n);
    }

    // Reads a scalar as an integer if it is one; skips anything else (strings, floats, arrays).
    QVariant readValue(quint32 type)
    {
        switch (type) {
        case U8: return read<quint8>();
        case I8: return read<qint8>();
        case U16: return read<quint16>();
        case I16: return read<qint16>();
        case U32: return read<quint32>();
        case I32: return read<qint32>();
        case U64: return read<quint64>();
        case I64: return read<qint64>();
        case BOOL: return bool(read<quint8>());
        case F32: skip(4); return {};
        case F64: skip(8); return {};
        case STRING: return readString();
        case ARRAY: {
            const quint32 elemType = read<quint32>();
            const quint64 count = read<quint64>();
            const int size = scalarSize(elemType);
            if (size > 0) {
                if (count > quint64(m_size)) { m_ok = false; return {}; }
                skip(count * quint64(size));
            } else {
                for (quint64 i = 0; i < count && m_ok; ++i)
                    readValue(elemType);
            }
            return {};
        }
        default:
            m_ok = false;
            return {};
        }
    }

private:
    bool need(quint64 n)
    {
        if (!m_ok || n > quint64(m_size - m_pos)) {
            m_ok = false;
            return false;
        }
        return true;
    }

    const uchar *m_data;
    qint64 m_size;
    qint64 m_pos = 0;
    bool m_ok = true;
};

const QRegularExpression &splitPattern()
{
    static const QRegularExpression re(QStringLiteral("^(.*)-(\\d{5})-of-(\\d{5})\\.gguf$"),
                                       QRegularExpression::CaseInsensitiveOption);
    return re;
}

} // namespace

bool isAuxiliaryGgufFile(const QString &fileName)
{
    if (fileName.startsWith(QLatin1String("mmproj"), Qt::CaseInsensitive))
        return true;
    const auto m = splitPattern().match(fileName);
    return m.hasMatch() && m.captured(2).toInt() != 1;
}

QString formatBytes(qint64 bytes)
{
    const double gib = double(bytes) / (1024.0 * 1024.0 * 1024.0);
    if (gib >= 1.0)
        return QStringLiteral("%1 GiB").arg(gib, 0, 'f', 1);
    return QStringLiteral("%1 MiB").arg(double(bytes) / (1024.0 * 1024.0), 0, 'f', 0);
}

GgufInfo readGgufInfo(const QString &path)
{
    GgufInfo info;
    QFileInfo fi(path);
    info.totalSize = fi.size();

    // Split models ("name-00001-of-00003.gguf"): weights are spread over all parts.
    const auto split = splitPattern().match(fi.fileName());
    if (split.hasMatch()) {
        info.splitCount = split.captured(3).toInt();
        info.totalSize = 0;
        for (int i = 1; i <= info.splitCount; ++i) {
            const QString part = QStringLiteral("%1-%2-of-%3.gguf")
                                     .arg(split.captured(1))
                                     .arg(i, 5, 10, QLatin1Char('0'))
                                     .arg(split.captured(3));
            info.totalSize += QFileInfo(fi.dir().filePath(part)).size();
        }
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        info.error = file.errorString();
        return info;
    }
    // Mapping the whole file costs nothing until pages are touched; only the header is read.
    const qint64 size = file.size();
    const uchar *data = file.map(0, size);
    if (!data) {
        info.error = file.errorString();
        return info;
    }

    Reader r(data, size);
    if (r.read<quint32>() != 0x46554747u) { // "GGUF"
        info.error = QStringLiteral("not a GGUF file");
        return info;
    }
    const quint32 version = r.read<quint32>();
    if (version < 2) {
        info.error = QStringLiteral("unsupported GGUF version %1").arg(version);
        return info;
    }
    r.read<quint64>(); // tensor count
    const quint64 kvCount = r.read<quint64>();

    QHash<QString, QVariant> kv;
    for (quint64 i = 0; i < kvCount && r.ok(); ++i) {
        const QString key = r.readString();
        const quint32 type = r.read<quint32>();
        const QVariant value = r.readValue(type);
        if (value.isValid())
            kv.insert(key, value);
    }
    if (!r.ok()) {
        info.error = QStringLiteral("truncated or corrupt GGUF header");
        return info;
    }

    info.architecture = kv.value(QStringLiteral("general.architecture")).toString();
    info.name = kv.value(QStringLiteral("general.name")).toString();
    info.sizeLabel = kv.value(QStringLiteral("general.size_label")).toString();
    const QString a = info.architecture;
    info.contextLength = kv.value(a + QStringLiteral(".context_length")).toULongLong();
    info.blockCount = kv.value(a + QStringLiteral(".block_count")).toULongLong();
    info.expertCount = kv.value(a + QStringLiteral(".expert_count")).toULongLong();
    info.expertUsedCount = kv.value(a + QStringLiteral(".expert_used_count")).toULongLong();
    info.valid = true;
    return info;
}
