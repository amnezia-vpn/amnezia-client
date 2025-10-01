#!/bin/sh

# This is default bat run script of The CQtDeployer project.
# This file contains key word that will replaced after deploy project.
#
# ####################################################################
#
# LIB_PATH - are relative path to libraries of a deployed distribution.
# QML_PATH - are relative path to qml libraries of a deployed distribution.
# PLUGIN_PATH - are relative path to qt plugins of a deployed distribution.
# BIN_PATH - are relative path to targets of a deployed distribution.

# SYSTEM_LIB_PATH - are relative path to system libraries of a deployed distribution.
# BASE_NAME - are base name of the executable that will be launched after run this script.
# CUSTOM_SCRIPT_BLOCK - This is code from the customScript option
# RUN_COMMAND - This is command for run application. Required BASE_DIR variable.
#
# ####################################################################

BASE_DIR=$(dirname "$(readlink -f "$0")")
export LD_LIBRARY_PATH="$BASE_DIR"/lib/:"$BASE_DIR":$LD_LIBRARY_PATH
export QML_IMPORT_PATH="$BASE_DIR"/qml/:$QML_IMPORT_PATH
export QML2_IMPORT_PATH="$BASE_DIR"/qml/:$QML2_IMPORT_PATH
export QT_PLUGIN_PATH="$BASE_DIR"/plugins/:$QT_PLUGIN_PATH
export QTWEBENGINEPROCESS_PATH="$BASE_DIR"/bin//QtWebEngineProcess
export QTDIR="$BASE_DIR"
export CQT_PKG_ROOT="$BASE_DIR"
export CQT_RUN_FILE="$BASE_DIR/AmneziaVPN.sh"

export QT_QPA_PLATFORM_PLUGIN_PATH="$BASE_DIR"/plugins//platforms:$QT_QPA_PLATFORM_PLUGIN_PATH



"$BASE_DIR/bin/AmneziaVPN" "$@" 
