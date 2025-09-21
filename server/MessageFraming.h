// utility to pack/unpack JSON with 4byte big-end prefix.
#pragma once
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

inline QByteArray packJson(const QJsonObject &obj) {
    QJsonDocument doc(obj);
    QByteArray payload = doc.toJson(QJsonDocument::Compact);
    QByteArray out;
    // Write 4-byte big-endian length
    quint32 len = static_cast<quint32>(payload.size());
    out.resize(4);
    out[0] = static_cast<char>((len >> 24) & 0xFF);
    out[1] = static_cast<char>((len >> 16) & 0xFF);
    out[2] = static_cast<char>((len >> 8) & 0xFF);
    out[3] = static_cast<char>((len) & 0xFF);
    out.append(payload);
    return out;
}

// try extract JSON from buffer; returns true on success and put in doc
// buffer is modified
inline bool tryExtractOne(QByteArray &buffer, QJsonDocument &docOut) {
    if (buffer.size() < 4) return false;
    const unsigned char *data = reinterpret_cast<const unsigned char*>(buffer.constData());
    quint32 len = (static_cast<quint32>(data[0]) << 24) |
                  (static_cast<quint32>(data[1]) << 16) |
                  (static_cast<quint32>(data[2]) << 8) |
                  (static_cast<quint32>(data[3]));
    if (buffer.size() < static_cast<int>(4 + len)) return false;
    QByteArray payload = buffer.mid(4, len);
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError) {
        // consume the bytes and return false (caller should handle)
        buffer.remove(0, 4 + len);
        return false;
    }
    docOut = doc;
    buffer.remove(0, 4 + len);
    return true;
}
