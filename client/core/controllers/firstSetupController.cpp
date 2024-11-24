#include "firstSetupController.h"

#include <QDebug>
#include <string>

#ifdef Q_OS_MACOS
#include <AmneziaVPN-Swift.h>
#include <core/ipcclient.h>
#endif

FirstSetupController::FirstSetupController(QObject *parent) : QObject{parent} {}

bool FirstSetupController::firstSetupNeeded() {
#ifdef Q_OS_MACOS
  return ZloVPN::firstSetupNeeded();
#endif

  return false;
}

void FirstSetupController::doFirstSetup() {
#ifdef Q_OS_MACOS
  QString firstSetupString = tr("To install the VPN service");
  ZloVPN::FirstSetupResponse response = ZloVPN::doFirstSetup(firstSetupString.toStdString());

  if (response.isError()) {
    std::string errorString = response.getErrorString();
    emit firstSetupFailed(response.requiresApproval(), QString::fromStdString(errorString));
    return;
  } else {
    IpcClient::Reinitialize();
  }
#endif

  emit firstSetupFinished();
}
  
void FirstSetupController::restartService() {
#ifdef Q_OS_MACOS
  auto restartResult = ZloVPN::restartService();
  if (restartResult.getErrorMessage()) {
    qDebug() << "Failed to start service, reason: " << restartResult.getErrorMessage().get();
  } else if (restartResult.getLauncherResponse()) {
    qDebug() << "Failed to start service, launcher response: " << restartResult.getLauncherResponse().get().getRawValue();
  }
#endif
}
