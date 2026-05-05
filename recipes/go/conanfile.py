from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import get, copy

import os

class Golang(ConanFile):
    name = "go"
    version = "1.26.0"
    
    settings = "os", "arch"

    def layout(self):
        basic_layout(self)
    
    def build(self):
        get(self, **self.conan_data["sources"][str(self.version)][str(self.settings.os)][str(self.settings.arch)], strip_root=True, destination="go")

    def package(self):
        copy(self, "*", src=os.path.join(self.build_folder, "go"), dst=self.package_folder)

    def package_info(self):
        self.cpp_info.bindirs = ["bin"]
