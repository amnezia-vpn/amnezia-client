from conan import ConanFile
from conan.tools.files import get, copy
from conan.tools.cmake import cmake_layout

import os

class Golang(ConanFile):
    name = "go"
    version = "1.26.0"
    
    settings = "os", "arch"

    def build_requirements(self):
        if self.settings.os == "Windows":
            self.tool_requires("mingw-builds/15.1.0")
    
    def build(self):
        get(self, **self.conan_data["sources"][str(self.version)][str(self.settings.os)][str(self.settings.arch)])

    def package(self):
        copy(self, "*", src=os.path.join(self.source_folder, "go"), dst=self.package_folder)

    def package_info(self):
        self.cpp_info.bindirs = ["bin"]
