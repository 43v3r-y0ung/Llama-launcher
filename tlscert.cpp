#include "tlscert.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <QLocale>
#include <QCryptographicHash>
#include <QProcess>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

namespace TlsCert {

namespace {

constexpr int kCaDays = 3650;
constexpr int kServerDays = 825;        // the longest validity Apple devices accept for TLS server certificates
constexpr int kRenewBeforeDays = 30;
constexpr int kOpensslTimeoutMs = 20000;

QString caKeyPath()
{
    return directory() + QStringLiteral("/ca-key.pem");
}

QString opensslExecutable()
{
    return QStandardPaths::findExecutable(QStringLiteral("openssl"));
}

bool runOpenssl(const QStringList &args, QString *error)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(opensslExecutable(), args);
    if (!process.waitForFinished(kOpensslTimeoutMs)) {
        process.kill();
        process.waitForFinished();
        *error = QCoreApplication::translate("TlsCert", "openssl did not finish: %1").arg(process.errorString());
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        *error = QCoreApplication::translate("TlsCert", "openssl failed: %1").arg(QString::fromLocal8Bit(process.readAll()).trimmed());
        return false;
    }
    return true;
}

bool writeFile(const QString &path, const QByteArray &data, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(data) != data.size()) {
        *error = QCoreApplication::translate("TlsCert", "Cannot write %1: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

void restrictToOwner(const QString &path)
{
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

QString randomSerial()
{
    // 64 random bits, positive (top bit clear), as openssl expects for -set_serial.
    const quint64 serial = QRandomGenerator::system()->generate64() & 0x7fffffffffffffffULL;
    return QStringLiteral("0x%1").arg(serial, 16, 16, QLatin1Char('0'));
}

bool generateCa(const QString &workDir, QString *error)
{
    const QString config = workDir + QStringLiteral("/ca.cnf");
    if (!writeFile(config,
                   "[req]\n"
                   "distinguished_name = dn\n"
                   "prompt = no\n"
                   "[dn]\n"
                   "[v3_ca]\n"
                   "basicConstraints = critical, CA:TRUE, pathlen:0\n"
                   "keyUsage = critical, keyCertSign, cRLSign\n"
                   "subjectKeyIdentifier = hash\n",
                   error))
        return false;

    const QString subject = QStringLiteral("/O=Llama Launcher/CN=Llama Launcher local CA (%1)")
                                .arg(QHostInfo::localHostName());
    if (!runOpenssl({QStringLiteral("req"), QStringLiteral("-x509"), QStringLiteral("-new"),
                     QStringLiteral("-newkey"), QStringLiteral("ec"),
                     QStringLiteral("-pkeyopt"), QStringLiteral("ec_paramgen_curve:prime256v1"),
                     QStringLiteral("-noenc"),
                     QStringLiteral("-keyout"), caKeyPath(), QStringLiteral("-out"), caCertPath(),
                     QStringLiteral("-days"), QString::number(kCaDays),
                     QStringLiteral("-subj"), subject,
                     QStringLiteral("-set_serial"), randomSerial(),
                     QStringLiteral("-config"), config, QStringLiteral("-extensions"), QStringLiteral("v3_ca")},
                    error)) {
        QFile::remove(caKeyPath());
        QFile::remove(caCertPath());
        return false;
    }
    restrictToOwner(caKeyPath());
    return true;
}

bool generateServer(const Names &names, const QString &workDir, QString *error)
{
    QByteArray alt;
    for (int i = 0; i < names.dns.size(); ++i)
        alt += QStringLiteral("DNS.%1 = %2\n").arg(i + 1).arg(names.dns.at(i)).toUtf8();
    for (int i = 0; i < names.ips.size(); ++i)
        alt += QStringLiteral("IP.%1 = %2\n").arg(i + 1).arg(names.ips.at(i)).toUtf8();

    const QString config = workDir + QStringLiteral("/server.cnf");
    if (!writeFile(config,
                   "[req]\n"
                   "distinguished_name = dn\n"
                   "prompt = no\n"
                   "[dn]\n"
                   "[v3_server]\n"
                   "basicConstraints = critical, CA:FALSE\n"
                   "keyUsage = critical, digitalSignature\n"
                   "extendedKeyUsage = serverAuth\n"
                   "subjectKeyIdentifier = hash\n"
                   "authorityKeyIdentifier = keyid\n"
                   "subjectAltName = @alt\n"
                   "[alt]\n" + alt,
                   error))
        return false;

    // Write to temporary files first so a failure leaves the previous certificate in place.
    const QString key = workDir + QStringLiteral("/server-key.pem");
    const QString csr = workDir + QStringLiteral("/server.csr");
    const QString cert = workDir + QStringLiteral("/server.pem");
    const QString subject = QStringLiteral("/O=Llama Launcher/CN=%1").arg(names.dns.value(1, QStringLiteral("localhost")));
    if (!runOpenssl({QStringLiteral("req"), QStringLiteral("-new"),
                     QStringLiteral("-newkey"), QStringLiteral("ec"),
                     QStringLiteral("-pkeyopt"), QStringLiteral("ec_paramgen_curve:prime256v1"),
                     QStringLiteral("-noenc"), QStringLiteral("-keyout"), key, QStringLiteral("-out"), csr,
                     QStringLiteral("-subj"), subject, QStringLiteral("-config"), config},
                    error))
        return false;
    if (!runOpenssl({QStringLiteral("x509"), QStringLiteral("-req"), QStringLiteral("-in"), csr,
                     QStringLiteral("-CA"), caCertPath(), QStringLiteral("-CAkey"), caKeyPath(),
                     QStringLiteral("-set_serial"), randomSerial(),
                     QStringLiteral("-days"), QString::number(kServerDays), QStringLiteral("-sha256"),
                     QStringLiteral("-extfile"), config, QStringLiteral("-extensions"), QStringLiteral("v3_server"),
                     QStringLiteral("-out"), cert},
                    error))
        return false;

    // Serve the full chain so clients that trust the CA can build the path.
    QFile certFile(cert), caFile(caCertPath());
    if (!certFile.open(QIODevice::ReadOnly) || !caFile.open(QIODevice::ReadOnly)) {
        *error = QCoreApplication::translate("TlsCert", "Cannot read the generated certificate.");
        return false;
    }
    QByteArray chain = certFile.readAll();
    if (!chain.endsWith('\n'))
        chain += '\n';
    chain += caFile.readAll();

    restrictToOwner(key);
    QFile::remove(serverKeyPath());
    if (!QFile::copy(key, serverKeyPath())) {
        *error = QCoreApplication::translate("TlsCert", "Cannot write %1").arg(serverKeyPath());
        return false;
    }
    restrictToOwner(serverKeyPath());
    return writeFile(serverCertPath(), chain, error);
}

} // namespace

QString directory()
{
    // Next to the settings file: ~/.config/LlamaLauncher/tls
    return QFileInfo(QSettings().fileName()).absolutePath() + QStringLiteral("/tls");
}

QString caCertPath()
{
    return directory() + QStringLiteral("/ca.pem");
}

QString serverCertPath()
{
    return directory() + QStringLiteral("/server.pem");
}

QString serverKeyPath()
{
    return directory() + QStringLiteral("/server-key.pem");
}

bool opensslAvailable()
{
    return !opensslExecutable().isEmpty();
}

Names currentNames(const QStringList &lanAddresses)
{
    Names names;
    names.dns << QStringLiteral("localhost");
    const QString host = QHostInfo::localHostName();
    if (!host.isEmpty() && host != QLatin1String("localhost")) {
        names.dns << host;
        if (!host.contains(QLatin1Char('.')))
            names.dns << host + QStringLiteral(".local"); // mDNS name (Avahi)
    }
    names.ips << QStringLiteral("127.0.0.1");
    for (const QString &ip : lanAddresses)
        if (!names.ips.contains(ip))
            names.ips << ip;
    return names;
}

QString renewalReason(const Names &names)
{
    if (!QFile::exists(caCertPath()) || !QFile::exists(caKeyPath()))
        return QCoreApplication::translate("TlsCert", "no certificate yet");
    const QSslCertificate ca = firstCertificate(caCertPath());
    const QSslCertificate cert = firstCertificate(serverCertPath());
    if (ca.isNull() || cert.isNull() || !QFile::exists(serverKeyPath()))
        return QCoreApplication::translate("TlsCert", "no certificate yet");
    if (cert.issuerInfo(QSslCertificate::CommonName) != ca.subjectInfo(QSslCertificate::CommonName))
        return QCoreApplication::translate("TlsCert", "issued by a different CA");

    const QDateTime soon = QDateTime::currentDateTimeUtc().addDays(kRenewBeforeDays);
    if (cert.expiryDate() < soon)
        return QCoreApplication::translate("TlsCert", "expires %1").arg(QLocale().toString(cert.expiryDate().toLocalTime().date(), QLocale::ShortFormat));

    const auto alt = cert.subjectAlternativeNames();
    const QStringList dns = alt.values(QSsl::DnsEntry);
    const QStringList ips = alt.values(QSsl::IpAddressEntry);
    for (const QString &name : names.dns)
        if (!dns.contains(name, Qt::CaseInsensitive))
            return QCoreApplication::translate("TlsCert", "does not cover %1").arg(name);
    for (const QString &ip : names.ips)
        if (!ips.contains(ip))
            return QCoreApplication::translate("TlsCert", "does not cover %1").arg(ip);
    return {};
}

bool generate(const Names &names, QString *error)
{
    if (!opensslAvailable()) {
        *error = QCoreApplication::translate("TlsCert", "The openssl program was not found. Install the openssl package.");
        return false;
    }
    if (!QDir().mkpath(directory())) {
        *error = QCoreApplication::translate("TlsCert", "Cannot create %1").arg(directory());
        return false;
    }
    QFile::setPermissions(directory(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    QTemporaryDir work;
    if (!work.isValid()) {
        *error = QCoreApplication::translate("TlsCert", "Cannot create a temporary folder: %1").arg(work.errorString());
        return false;
    }
    QFile::setPermissions(work.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    const bool haveCa = !firstCertificate(caCertPath()).isNull() && QFile::exists(caKeyPath())
                        && firstCertificate(caCertPath()).expiryDate()
                               > QDateTime::currentDateTimeUtc().addDays(kServerDays);
    if (!haveCa && !generateCa(work.path(), error))
        return false;
    return generateServer(names, work.path(), error);
}

QSslCertificate firstCertificate(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QSslCertificate();
    const QList<QSslCertificate> certs = QSslCertificate::fromDevice(&file, QSsl::Pem);
    return certs.isEmpty() ? QSslCertificate() : certs.first();
}

QString fingerprint(const QSslCertificate &cert)
{
    if (cert.isNull())
        return {};
    return QString::fromLatin1(cert.digest(QCryptographicHash::Sha256).toHex(':').toUpper());
}

} // namespace TlsCert
