#ifndef TLSCERT_H
#define TLSCERT_H

#include <QSslCertificate>
#include <QString>
#include <QStringList>

// Certificates the launcher creates for HTTPS, with the openssl command-line tool.
//
// A small local CA signs the server certificate. Clients import the CA once; when the
// machine's address changes only the server certificate is reissued, so they keep trusting it.
namespace TlsCert {

QString directory();          // ~/.config/LlamaLauncher/tls, owner-only
QString caCertPath();         // what clients import
QString serverCertPath();     // server certificate followed by the CA (full chain)
QString serverKeyPath();

bool opensslAvailable();

// Names and addresses the server certificate must cover: localhost, 127.0.0.1,
// the host name and every LAN IPv4 address.
struct Names {
    QStringList dns;
    QStringList ips;
};
Names currentNames(const QStringList &lanAddresses);

// Why the managed server certificate needs to be (re)issued, or an empty string if it is fine.
QString renewalReason(const Names &names);

// Creates the CA if it does not exist yet, then issues a new server certificate.
bool generate(const Names &names, QString *error);

// First certificate in a PEM file (the server's own), or a null certificate.
QSslCertificate firstCertificate(const QString &path);

// SHA-256 fingerprint as AA:BB:..., or an empty string.
QString fingerprint(const QSslCertificate &cert);

} // namespace TlsCert

#endif // TLSCERT_H
