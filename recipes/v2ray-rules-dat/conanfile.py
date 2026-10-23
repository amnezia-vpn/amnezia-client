from conan import ConanFile
from conan.tools.layout import basic_layout
from conan.tools.files import download, copy

import os

class V2rayRulesDat(ConanFile):
    name = "v2ray-rules-dat"
    version = "202603162227"

    def layout(self):
        basic_layout(self, build_folder=".")

    def source(self):
        # TODO(ygurov): build from source instead of plain copying
        download(self, filename="geoip.dat", url=f"https://github.com/Loyalsoldier/v2ray-rules-dat/releases/download/{self.version}/geoip.dat",
            sha256="e48b925d985d7bf33cfee76f309241af0f1779699963b69363dec2c4740041d1")
        download(self, filename="geosite.dat", url=f"https://github.com/Loyalsoldier/v2ray-rules-dat/releases/download/{self.version}/geosite.dat",
            sha256="a2f83e25b8be3f089cfdd9423fc7b6eda5a0d4060919917902711d65acac1e0c")

    def package(self):
        copy(self, "*.dat", src=self.build_folder, dst=self.package_folder)

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "Loyalsoldier::v2ray-rules-dat")
        self.cpp_info.set_property("cmake_extra_variables", {
            "GEOSITE_DAT_PATH": os.path.join(self.package_folder, "geosite.dat").replace("\\", "/"),
            "GEOIP_DAT_PATH": os.path.join(self.package_folder, "geoip.dat").replace("\\", "/")
        })
