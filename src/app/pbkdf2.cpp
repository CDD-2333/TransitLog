#include "pbkdf2.h"

#include <QCryptographicHash>
#include <QList>
#include <QRandomGenerator>

namespace {

QByteArray hmacSha256(const QByteArray& key, const QByteArray& msg)
{
    const int blockSize = 64;
    // 注意：QByteArray::resize() 增长部分不会清零，必须显式零填充，
    // 否则 key 不足 64 字节时 HMAC 结果依赖未初始化内存（UB）。
    QByteArray k(blockSize, char(0));
    {
        QByteArray kk = key;
        if (kk.size() > blockSize)
            kk = QCryptographicHash::hash(kk, QCryptographicHash::Sha256);
        const int n = qMin(kk.size(), blockSize);
        for (int i = 0; i < n; ++i)
            k[i] = kk.at(i);
    }

    QByteArray ipad(blockSize, char(0x36));
    QByteArray opad(blockSize, char(0x5c));
    for (int i = 0; i < blockSize; ++i) {
        ipad[i] = char(uchar(k.at(i)) ^ uchar(ipad.at(i)));
        opad[i] = char(uchar(k.at(i)) ^ uchar(opad.at(i)));
    }

    const QByteArray inner = QCryptographicHash::hash(ipad + msg, QCryptographicHash::Sha256);
    return QCryptographicHash::hash(opad + inner, QCryptographicHash::Sha256);
}

QByteArray pbkdf2Sha256(const QByteArray& password, const QByteArray& salt,
                        int iterations, int dkLen)
{
    QByteArray dk;
    quint32 blockIndex = 1;
    while (dk.size() < dkLen) {
        // 大端序块序号
        QByteArray ib;
        ib.append(char((blockIndex >> 24) & 0xFF));
        ib.append(char((blockIndex >> 16) & 0xFF));
        ib.append(char((blockIndex >> 8) & 0xFF));
        ib.append(char(blockIndex & 0xFF));

        QByteArray u = hmacSha256(password, salt + ib);
        QByteArray t = u;
        QByteArray prev = u;
        for (int i = 1; i < iterations; ++i) {
            prev = hmacSha256(password, prev);
            for (int j = 0; j < t.size(); ++j)
                t[j] = char(uchar(t.at(j)) ^ uchar(prev.at(j)));
        }
        dk += t;
        ++blockIndex;
    }
    return dk.left(dkLen);
}

QByteArray randomSalt(int size)
{
    QByteArray salt(size, char(0));
    for (int i = 0; i < size; ++i)
        salt[i] = char(QRandomGenerator::system()->bounded(256));
    return salt;
}

} // namespace

namespace PBKDF2 {

QByteArray hashPassword(const QString& password)
{
    const int iterations = 120000;
    const QByteArray salt = randomSalt(16);
    const QByteArray dk = pbkdf2Sha256(password.toUtf8(), salt, iterations, 32);
    return QByteArray("pbkdf2$") + QByteArray::number(iterations) + '$'
         + salt.toHex() + '$' + dk.toHex();
}

bool verifyPassword(const QString& password, const QByteArray& stored)
{
    const QList<QByteArray> parts = stored.split('$');
    if (parts.size() != 4 || parts.at(0) != "pbkdf2")
        return false;

    bool ok = false;
    const int iterations = parts.at(1).toInt(&ok);
    if (!ok || iterations <= 0)
        return false;

    const QByteArray salt = QByteArray::fromHex(parts.at(2));
    const QByteArray expected = QByteArray::fromHex(parts.at(3));
    const QByteArray calc = pbkdf2Sha256(password.toUtf8(), salt, iterations, expected.size());

    if (calc.size() != expected.size())
        return false;
    // 常量时间比较
    uchar diff = 0;
    for (int i = 0; i < calc.size(); ++i)
        diff |= uchar(calc.at(i)) ^ uchar(expected.at(i));
    return diff == 0;
}

} // namespace PBKDF2
