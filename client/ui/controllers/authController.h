#ifndef AUTHCONTROLLER_H
#define AUTHCONTROLLER_H

#include "errorParser.h"
#include "settings.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <optional>

struct Response {
  int statusCode;
  std::optional<Errors> errors{};

  static bool isOk(int statusCode) { return (statusCode / 100) == 2; }
  [[nodiscard]] bool isOk() const {
    return isOk(statusCode) && !errors.has_value();
  }
};

struct UserInfo {
  Q_GADGET

  Q_PROPERTY(QString username MEMBER username)
  Q_PROPERTY(QString email MEMBER email)
  Q_PROPERTY(qint64 timeLeft MEMBER timeLeft)
  Q_PROPERTY(QString localizedTimeLeft READ localizeTimeLeft)
  Q_PROPERTY(bool isValid MEMBER isValid)

public slots:
  QString localizeTimeLeft() const;
  qint64 monthsAvailableToAdd() const;

public:
  bool operator==(const UserInfo &other) const {
    return username == other.username && timeLeft == other.timeLeft;
  }

  bool operator!=(const UserInfo &other) const { return !(*this == other); }

  QString username;
  QString email;
  qint64 timeLeft;
  bool isValid{false};
};

struct RegionInfo {
  Q_GADGET

  Q_PROPERTY(QString id MEMBER id)
  Q_PROPERTY(QString countryCode MEMBER countryCode)
  Q_PROPERTY(QString countryName MEMBER countryName)

public:
  bool operator==(const RegionInfo &other) const {
    return id == other.id && countryCode == other.countryCode &&
           countryName == other.countryName;
  }

  bool operator!=(const RegionInfo &other) const { return !(*this == other); }

  QString id;
  QString countryCode;
  QString countryName;
};

class ServerStringRequest : public QObject {
  Q_OBJECT

public:
  explicit ServerStringRequest(QObject *parent = nullptr) : QObject{parent} {};

signals:
  void stringArrived(const QString connectionString);
  void errorOccurred(const Errors errors);
};

class AuthController : public QObject {
  Q_OBJECT
  Q_PROPERTY(UserInfo userInfo MEMBER m_userInfo NOTIFY userInfoUpdated);
  Q_PROPERTY(
      QList<RegionInfo> regionInfo MEMBER m_regions NOTIFY regionsUpdated);
  Q_PROPERTY(bool spikeReady READ isSpikeReady NOTIFY spikeUpdated);
  Q_PROPERTY(bool spikeErrored MEMBER m_spikeErrored NOTIFY spikeUpdated);
  Q_PROPERTY(bool updateRequired MEMBER m_updateRequired NOTIFY
                 apiCompatibilityChanged);

public:
  explicit AuthController(std::shared_ptr<Settings> settings,
                          QObject *parent = nullptr);

  const QList<RegionInfo> &getRegions() const { return m_regions; }

public slots:
  bool isAuthenticated();
  bool hasToken();
  QString getToken();
  void setToken(const QString &token);
  void setUnauthenticated();

  void refreshToken();
  void login(const QString &login, const QString &password);
  void registerUser(const QString &email, const QString &username,
                    const QString &password);
  void recoverAccount(const QString &email);
  void changePassword(const QString &currentPassword,
                      const QString &newPassword);
  void changeEmail(const QString &newEmail);
  void checkApiCompatibility();
  void logout();

  void refreshUserInfo();
  void refreshServers();

  QString getSelectedRegionId();
  void setSelectedRegionId(QString regionId);

  void getServerConnectionString(const QString &serverId,
                                 ServerStringRequest *stringRequest);
  void openPaymentLink();

  bool isSpikeReady();
  QString getSpikeUrl();

signals:
  void apiCompatibilityChanged();
  void spikeErrorOccurred();
  void errorOccurred(const Errors errors);
  void errorOccurredQml(const QString errorMessage,
                        const QVariantMap fieldErrors);
  void tokenUpdated(bool authenticationStateChanged);
  void loginSuccessfull();
  void registerSuccessfull();
  void recoveryEmailSent();
  void passwordChanged();
  void emailChanged();
  void spikeUpdated();

  void userInfoUpdated();
  void regionsUpdated();
  void paymentLinkOpened();

private:
  QNetworkRequest createNetworkRequest(const QString &endpoint,
                                       bool needsAuthorization = false,
                                       const QByteArray *array = nullptr);
  Response parseNetworkReply(QByteArray &data, QNetworkReply &reply,
                             bool formError = false);

  std::shared_ptr<Settings> m_settings;

  QString m_token{};
  UserInfo m_userInfo{};
  QList<RegionInfo> m_regions{};
  QScopedPointer<QNetworkAccessManager> m_qnam;
  QString m_spike{};
  bool m_authenticated{};
  bool m_spikeErrored{};
  bool m_updateRequired{};
};

#endif // AUTHCONTROLLER_H
