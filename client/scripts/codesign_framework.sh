#!/bin/sh

# WARNING: You may have to run Clean in Xcode after changing CODE_SIGN_IDENTITY! 

# Verify that $CODE_SIGN_IDENTITY is set
if [ -z "${CODE_SIGN_IDENTITY}" ] ; then
    echo "CODE_SIGN_IDENTITY needs to be set for framework code-signing!"
    
    if [ "${CONFIGURATION}" = "Release" ] ; then
        exit 1
    else
        # Code-signing is optional for non-release builds.
        exit 0
    fi
fi

if [ -z "${CODE_SIGN_ENTITLEMENTS}" ] ; then
    echo "CODE_SIGN_ENTITLEMENTS needs to be set for framework code-signing!"
    
    if [ "${CONFIGURATION}" = "Release" ] ; then
        exit 1
    else
        # Code-signing is optional for non-release builds.
        exit 0
    fi
fi

ITEMS=""

FRAMEWORKS_DIR="${TARGET_BUILD_DIR}/${FRAMEWORKS_FOLDER_PATH}"
if [ -d "$FRAMEWORKS_DIR" ] ; then
    FRAMEWORKS=$(find "${FRAMEWORKS_DIR}" -depth -type d -name "*.framework" -or -name "*.dylib" -or -name "*.bundle" | sed -e "s/\(.*framework\)/\1\/Versions\/A\//")
    RESULT=$?
    if [[ $RESULT != 0 ]] ; then
        exit 1
    fi
    
    ITEMS="${FRAMEWORKS}"
fi

LOGINITEMS_DIR="${TARGET_BUILD_DIR}/${CONTENTS_FOLDER_PATH}/Library/LoginItems/"
if [ -d "$LOGINITEMS_DIR" ] ; then
    LOGINITEMS=$(find "${LOGINITEMS_DIR}" -depth -type d -name "*.app")
    RESULT=$?
    if [[ $RESULT != 0 ]] ; then
        exit 1
    fi
    
    ITEMS="${ITEMS}"$'\n'"${LOGINITEMS}"
fi

# Prefer the expanded name, if available.
CODE_SIGN_IDENTITY_FOR_ITEMS="${EXPANDED_CODE_SIGN_IDENTITY_NAME}"
if [ "${CODE_SIGN_IDENTITY_FOR_ITEMS}" = "" ] ; then
    # Fall back to old behavior.
    CODE_SIGN_IDENTITY_FOR_ITEMS="${CODE_SIGN_IDENTITY}"
fi

echo "Identity:"
echo "${CODE_SIGN_IDENTITY_FOR_ITEMS}"

echo "Entitlements:"
echo "${CODE_SIGN_ENTITLEMENTS}"

echo "Found:"
echo "${ITEMS}"

# Change the Internal Field Separator (IFS) so that spaces in paths will not cause problems below.
SAVED_IFS=$IFS
IFS=$(echo -en "\n\b")

# Loop through all items.
for ITEM in $ITEMS;
do
    echo "Signing '${ITEM}'"
    codesign --force --verbose --sign "${CODE_SIGN_IDENTITY_FOR_ITEMS}" --entitlements "${CODE_SIGN_ENTITLEMENTS}" "${ITEM}"
    RESULT=$?
    if [[ $RESULT != 0 ]] ; then
        echo "Failed to sign '${ITEM}'."
        IFS=$SAVED_IFS
        exit 1
    fi
done

# Restore $IFS.
IFS=$SAVED_IFS

# Save it to a file in your project.
# Mine is called codesign-frameworks.sh.
# Add a “Run Script” build phase right after your “Copy Embedded Frameworks” build phase.
# You can call it “Codesign Embedded Frameworks”.
# Paste ./codesign-frameworks.sh (or whatever you called your script above) into the script editor text field.
# Build your app. All bundled frameworks will be codesigned.
# from http://stackoverflow.com/questions/7697508/how-do-you-codesign-framework-bundles-for-the-mac-app-store