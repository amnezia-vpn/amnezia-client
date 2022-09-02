#!/bin/sh

# add `@executable_path/../../Frameworks` to Runpath Search Paths in Build Settings of extension
# put to run script phase in build phases for extension
# Type a script or drag a script file from your workspace to insert its path.
cd "${CONFIGURATION_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/"
if [[ -d "Frameworks" ]]; then 
	rm -fr Frameworks
fi