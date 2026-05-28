from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import fix_apple_shared_install_name, is_apple_os, XCRun
from conan.tools.build import build_jobs
from conan.tools.files import chdir, copy, get, replace_in_file, rm, rmdir, save
from conan.tools.gnu import AutotoolsToolchain
from conan.tools.layout import basic_layout
from conan.tools.microsoft import is_msvc, msvc_runtime_flag, unix_path
from conan.tools.scm import Version

import fnmatch
import os
import textwrap

required_conan_version = ">=1.57.0"


class OpenSSLConan(ConanFile):
    name = "openssl"
    version = "3.6.2"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/openssl/openssl"
    license = "Apache-2.0"
    topics = ("ssl", "tls", "encryption", "security")
    description = "A toolkit for the Transport Layer Security (TLS) and Secure Sockets Layer (SSL) protocols"
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "enable_weak_ssl_ciphers": [True, False],
        "386": [True, False],
        "capieng_dialog": [True, False],
        "enable_capieng": [True, False],
        "enable_trace": [True, False],
        "no_aria": [True, False],
        "no_apps": [True, False],
        "no_autoload_config": [True, False],
        "no_asm": [True, False],
        "no_async": [True, False],
        "no_blake2": [True, False],
        "no_bf": [True, False],
        "no_camellia": [True, False],
        "no_chacha": [True, False],
        "no_cms": [True, False],
        "no_comp": [True, False],
        "no_ct": [True, False],
        "no_cast": [True, False],
        "no_deprecated": [True, False],
        "no_des": [True, False],
        "no_dgram": [True, False],
        "no_dh": [True, False],
        "no_dsa": [True, False],
        "no_dso": [True, False],
        "no_ec": [True, False],
        "no_ecdh": [True, False],
        "no_ecdsa": [True, False],
        "no_engine": [True, False],
        "no_filenames": [True, False],
        "no_fips": [True, False],
        "no_gost": [True, False],
        "no_idea": [True, False],
        "no_legacy": [True, False],
        "no_md2": [True, False],
        "no_md4": [True, False],
        "no_mdc2": [True, False],
        "no_module": [True, False],
        "no_ocsp": [True, False],
        "no_pinshared": [True, False],
        "no_rc2": [True, False],
        "no_rc4": [True, False],
        "no_rc5": [True, False],
        "no_rfc3779": [True, False],
        "no_rmd160": [True, False],
        "no_sm2": [True, False],
        "no_sm3": [True, False],
        "no_sm4": [True, False],
        "no_srp": [True, False],
        "no_srtp": [True, False],
        "no_sse2": [True, False],
        "no_ssl": [True, False],
        "no_stdio": [True, False],
        "no_seed": [True, False],
        "no_sock": [True, False],
        "no_ssl3": [True, False],
        "no_threads": [True, False],
        "no_tls1": [True, False],
        "no_ts": [True, False],
        "no_whirlpool": [True, False],
        "no_zlib": [True, False],
        "openssldir": [None, "ANY"],
        "tls_security_level": [None, 0, 1, 2, 3, 4, 5],
    }
    default_options = {key: False for key in options.keys()}
    default_options["fPIC"] = True
    default_options["no_md2"] = True
    default_options["openssldir"] = None
    default_options["tls_security_level"] = None

    @property
    def _is_clang_cl(self):
        return self.settings.os == "Windows" and self.settings.compiler == "clang" and \
               self.settings.compiler.get_safe("runtime")

    @property
    def _is_mingw(self):
        return self.settings.os == "Windows" and self.settings.compiler == "gcc"

    @property
    def _use_nmake(self):
        return self._is_clang_cl or is_msvc(self)

    def config_options(self):
        if self.settings.os != "Windows":
            self.options.rm_safe("capieng_dialog")
            self.options.rm_safe("enable_capieng")
        else:
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        basic_layout(self, src_folder="src")

    def requirements(self):
        if not self.options.no_zlib:
            self.requires("zlib/[>=1.2.11 <2]")

    def validate(self):
        if self.settings.os == "iOS" and self.options.shared:
            raise ConanInvalidConfiguration("OpenSSL 3 does not support building shared libraries for iOS")

    def build_requirements(self):
        if self.settings_build.os == "Windows":
            if self.conf.get("user.openssl:windows_use_jom", False):
                self.tool_requires("jom/[*]")
            if not self.options.no_asm and self.settings.arch in ["x86", "x86_64"]:
                self.tool_requires("nasm/2.16.01")
            if self._use_nmake:
                self.tool_requires("strawberryperl/5.32.1.1")
            else:
                self.win_bash = True
                if not self.conf.get("tools.microsoft.bash:path", check_type=str):
                    self.tool_requires("msys2/cci.latest")

    def source(self):
        get(self, url="https://github.com/openssl/openssl/releases/download/openssl-3.6.2/openssl-3.6.2.tar.gz",
            sha256="aaf51a1fe064384f811daeaeb4ec4dce7340ec8bd893027eee676af31e83a04f", strip_root=True)

    @property
    def _target(self):
        target = f"conan-{self.settings.build_type}-{self.settings.os}-{self.settings.arch}-{self.settings.compiler}-{self.settings.compiler.version}"
        if self._use_nmake:
            target = f"VC-{target}"  # VC- prefix is important as it's checked by Configure
        if self._is_mingw:
            target = f"mingw-{target}"
        return target

    @property
    def _perlasm_scheme(self):
        # right now, we need to tweak this for iOS & Android only, as they inherit from generic targets
        if self.settings.os in ("iOS", "watchOS", "tvOS"):
            return {
                "armv7": "ios32",
                "armv7s": "ios32",
                "armv8": "ios64",
                "armv8_32": "ios64",
                "armv8.3": "ios64",
                "armv7k": "ios32",
            }.get(str(self.settings.arch), None)
        elif self.settings.os == "Android":
            return {
                "armv7": "void",
                "armv8": "linux64",
                "mips": "o32",
                "mips64": "64",
                "x86": "android",
                "x86_64": "elf",
            }.get(str(self.settings.arch), None)
        return None

    @property
    def _asm_target(self):
        if self.settings.os in ("Android", "iOS", "watchOS", "tvOS"):
            return {
                "x86": "x86_asm" if self.settings.os == "Android" else None,
                "x86_64": "x86_64_asm" if self.settings.os == "Android" else None,
                "armv5el": "armv4_asm",
                "armv5hf": "armv4_asm",
                "armv6": "armv4_asm",
                "armv7": "armv4_asm",
                "armv7hf": "armv4_asm",
                "armv7s": "armv4_asm",
                "armv7k": "armv4_asm",
                "armv8": "aarch64_asm",
                "armv8_32": "aarch64_asm",
                "armv8.3": "aarch64_asm",
                "mips": "mips32_asm",
                "mips64": "mips64_asm",
                "sparc": "sparcv8_asm",
                "sparcv9": "sparcv9_asm",
                "ia64": "ia64_asm",
                "ppc32be": "ppc32_asm",
                "ppc32": "ppc32_asm",
                "ppc64le": "ppc64_asm",
                "ppc64": "ppc64_asm",
                "s390": "s390x_asm",
                "s390x": "s390x_asm"
            }.get(str(self.settings.os), None)

    @property
    def _targets(self):
        is_cygwin = self.settings.get_safe("os.subsystem") == "cygwin"
        return {
            "Linux-x86-clang": "linux-x86-clang",
            "Linux-x86_64-clang": "linux-x86_64-clang",
            "Linux-x86-*": "linux-x86",
            "Linux-x86_64-*": "linux-x86_64",
            "Linux-armv4-*": "linux-armv4",
            "Linux-armv4i-*": "linux-armv4",
            "Linux-armv5el-*": "linux-armv4",
            "Linux-armv5hf-*": "linux-armv4",
            "Linux-armv6-*": "linux-armv4",
            "Linux-armv7-*": "linux-armv4",
            "Linux-armv7hf-*": "linux-armv4",
            "Linux-armv7s-*": "linux-armv4",
            "Linux-armv7k-*": "linux-armv4",
            "Linux-armv8-*": "linux-aarch64",
            "Linux-armv8.3-*": "linux-aarch64",
            "Linux-armv8-32-*": "linux-arm64ilp32",
            "Linux-mips-*": "linux-mips32",
            "Linux-mips64-*": "linux-mips64",
            "Linux-ppc32-*": "linux-ppc32",
            "Linux-ppc32le-*": "linux-pcc32",
            "Linux-ppc32be-*": "linux-ppc32",
            "Linux-ppc64-*": "linux-ppc64",
            "Linux-ppc64le-*": "linux-ppc64le",
            "Linux-pcc64be-*": "linux-pcc64",
            "Linux-s390x-*": "linux64-s390x",
            "Linux-e2k-*": "linux-generic64",
            "Linux-sparc-*": "linux-sparcv8",
            "Linux-sparcv9-*": "linux64-sparcv9",
            "Linux-*-*": "linux-generic32",
            "Macos-x86-*": "darwin-i386-cc",
            "Macos-x86_64-*": "darwin64-x86_64-cc",
            "Macos-ppc32-*": "darwin-ppc-cc",
            "Macos-ppc32be-*": "darwin-ppc-cc",
            "Macos-ppc64-*": "darwin64-ppc-cc",
            "Macos-ppc64be-*": "darwin64-ppc-cc",
            "Macos-armv8-*": "darwin64-arm64-cc",
            "Macos-*-*": "darwin-common",
            "iOS-x86_64-*": "darwin64-x86_64-cc",
            "iOS-*-*": "iphoneos-cross",
            "watchOS-*-*": "iphoneos-cross",
            "tvOS-*-*": "iphoneos-cross",
            # Android targets are very broken, see https://github.com/openssl/openssl/issues/7398
            "Android-armv7-*": "linux-generic32",
            "Android-armv7hf-*": "linux-generic32",
            "Android-armv8-*": "linux-generic64",
            "Android-x86-*": "linux-x86-clang",
            "Android-x86_64-*": "linux-x86_64-clang",
            "Android-mips-*": "linux-generic32",
            "Android-mips64-*": "linux-generic64",
            "Android-*-*": "linux-generic32",
            "Windows-x86-gcc": "Cygwin-x86" if is_cygwin else "mingw",
            "Windows-x86_64-gcc": "Cygwin-x86_64" if is_cygwin else "mingw64",
            "Windows-*-gcc": "Cygwin-common" if is_cygwin else "mingw-common",
            "Windows-ia64-Visual Studio": "VC-WIN64I",  # Itanium
            "Windows-x86-Visual Studio": "VC-WIN32",
            "Windows-x86_64-Visual Studio": "VC-WIN64A",
            "Windows-armv7-Visual Studio": "VC-WIN32-ARM",
            "Windows-armv8-Visual Studio": "VC-WIN64-CLANGASM-ARM",
            "Windows-*-Visual Studio": "VC-noCE-common",
            "Windows-ia64-clang": "VC-WIN64I",  # Itanium
            "Windows-x86-clang": "VC-WIN32",
            "Windows-x86_64-clang": "VC-WIN64A",
            "Windows-armv7-clang": "VC-WIN32-ARM",
            "Windows-armv8-clang": "VC-WIN64-ARM",
            "Windows-*-clang": "VC-noCE-common",
            "WindowsStore-x86-*": "VC-WIN32-UWP",
            "WindowsStore-x86_64-*": "VC-WIN64A-UWP",
            "WindowsStore-armv7-*": "VC-WIN32-ARM-UWP",
            "WindowsStore-armv8-*": "VC-WIN64-ARM-UWP",
            "WindowsStore-*-*": "VC-WIN32-ONECORE",
            "WindowsCE-*-*": "VC-CE",
            "SunOS-x86-gcc": "solaris-x86-gcc",
            "SunOS-x86_64-gcc": "solaris64-x86_64-gcc",
            "SunOS-sparc-gcc": "solaris-sparcv8-gcc",
            "SunOS-sparcv9-gcc": "solaris64-sparcv9-gcc",
            "SunOS-x86-suncc": "solaris-x86-cc",
            "SunOS-x86_64-suncc": "solaris64-x86_64-cc",
            "SunOS-sparc-suncc": "solaris-sparcv8-cc",
            "SunOS-sparcv9-suncc": "solaris64-sparcv9-cc",
            "SunOS-*-*": "solaris-common",
            "*BSD-x86-*": "BSD-x86",
            "*BSD-x86_64-*": "BSD-x86_64",
            "*BSD-ia64-*": "BSD-ia64",
            "*BSD-sparc-*": "BSD-sparcv8",
            "*BSD-sparcv9-*": "BSD-sparcv9",
            "*BSD-armv8-*": "BSD-generic64",
            "*BSD-mips64-*": "BSD-generic64",
            "*BSD-ppc64-*": "BSD-generic64",
            "*BSD-ppc64le-*": "BSD-generic64",
            "*BSD-ppc64be-*": "BSD-generic64",
            "AIX-ppc32-gcc": "aix-gcc",
            "AIX-ppc64-gcc": "aix64-gcc",
            "AIX-pcc32-*": "aix-cc",
            "AIX-ppc64-*": "aix64-cc",
            "AIX-*-*": "aix-common",
            "*BSD-*-*": "BSD-generic32",
            "Emscripten-*-*": "cc",
            "Neutrino-*-*": "BASE_unix",
        }

    @property
    def _ancestor_target(self):
        if "CONAN_OPENSSL_CONFIGURATION" in os.environ:
            return os.environ["CONAN_OPENSSL_CONFIGURATION"]
        compiler = "Visual Studio" if self.settings.compiler == "msvc" else self.settings.compiler
        query = f"{self.settings.os}-{self.settings.arch}-{compiler}"
        ancestor = next((self._targets[i] for i in self._targets if fnmatch.fnmatch(query, i)), None)
        if not ancestor:
            raise ConanInvalidConfiguration(
                f"Unsupported configuration ({self.settings.os}/{self.settings.arch}/{self.settings.compiler}).\n"
                f"Please open an issue at {self.url}.\n"
                f"Alternatively, set the CONAN_OPENSSL_CONFIGURATION environment variable into your conan profile."
            )
        return ancestor

    def _get_default_openssl_dir(self):
        if self.settings.os == "Linux":
            return "/etc/ssl"
        return os.path.join(self.package_folder, "res")

    def _adjust_path(self, path):
        if self._use_nmake:
            return path.replace("\\", "/")
        return unix_path(self, path)

    @property
    def _configure_args(self):
        openssldir = self.options.openssldir or self._get_default_openssl_dir()
        openssldir = unix_path(self, openssldir) if self.win_bash else openssldir
        args = [
            f'"{self._target}"',
            "shared" if self.options.shared else "no-shared",
            "--debug" if self.settings.build_type == "Debug" else "--release",
            "--prefix=/",
            "--libdir=lib",
            f"--openssldir=\"{openssldir}\"",
            "no-threads" if self.options.no_threads else "threads",
            f"PERL={self._perl}",
            "no-unit-test",
            "no-tests",
        ]

        if self.settings.os == "Android":
            args.append(f" -D__ANDROID_API__={str(self.settings.os.api_level)}")  # see NOTES.ANDROID
        if self.settings.os == "Windows":
            if self.options.enable_capieng:
                args.append("enable-capieng")
            if self.options.capieng_dialog:
                args.append("-DOPENSSL_CAPIENG_DIALOG=1")
        else:
            args.append("-fPIC" if self.options.get_safe("fPIC", True) else "no-pic")

        args.append("no-fips" if self.options.get_safe("no_fips", True) else "enable-fips")
        args.append("no-md2" if self.options.get_safe("no_md2", True) else "enable-md2")
        if str(self.options.tls_security_level) != "None":
            args.append(f"-DOPENSSL_TLS_SECURITY_LEVEL={self.options.tls_security_level}")

        if self.options.get_safe("enable_trace"):
            args.append("enable-trace")

        if self.settings.os == "Neutrino":
            args.append("no-asm -lsocket -latomic")

        if not self.options.no_zlib:
            zlib_cpp_info = self.dependencies["zlib"].cpp_info.aggregated_components()
            include_path = self._adjust_path(zlib_cpp_info.includedirs[0])
            is_shared_zlib = self.dependencies["zlib"].options.shared


            # the --with-zlib-lib flag takes a different value depending on platform and if ZLIB is shared
            # From https://github.com/openssl/openssl/blob/openssl-3.4.1/INSTALL.md#with-zlib-lib
            # On Unix: the directory where the zlib library is (for -L flag)
            # On Windows with static zlib: the path to the static library to link (assumed)
            # On Windows with shared zlib: the leaf name of the dll (its loaded with LoadLibrary)
            if self._use_nmake:
                # notes: consider where this should be "if on windows"
                #        zlib1 is assumed to be the name of the zlib1.dll for all windows configurations
                lib_path = self._adjust_path(os.path.join(zlib_cpp_info.libdirs[0], f"{zlib_cpp_info.libs[0]}.lib"))
                zlib_lib_flag = "zlib1" if is_shared_zlib else lib_path
            else:
                # Just path, GNU like compilers will find the right file
                zlib_lib_flag = self._adjust_path(zlib_cpp_info.libdirs[0])

            zlib_configure_arg = "zlib-dynamic" if is_shared_zlib else "zlib"
            args.append(zlib_configure_arg)

            args.extend([
                f'--with-zlib-include="{include_path}"',
                f'--with-zlib-lib="{zlib_lib_flag}"',
            ])

        for option_name in self.default_options.keys():
            if self.options.get_safe(option_name, False) and option_name not in ("shared", "fPIC", "openssldir", "tls_security_level", "capieng_dialog", "enable_capieng", "zlib", "no_fips", "no_md2"):
                self.output.info(f"Activated option: {option_name}")
                args.append(option_name.replace("_", "-"))
        return args

    def generate(self):
        tc = AutotoolsToolchain(self)
        env = tc.environment()
        env.define_path("PERL", self._perl)
        if self.settings.compiler == "apple-clang":
            xcrun = XCRun(self)
            env.define_path("CROSS_SDK", os.path.basename(xcrun.sdk_path))
            env.define_path("CROSS_TOP", os.path.dirname(os.path.dirname(xcrun.sdk_path)))

        if is_apple_os(self) and self.options.shared:
            # Inject -headerpad_max_install_names for shared library, otherwise fix_apple_shared_install_name() may fail.
            # See https://github.com/conan-io/conan-center-index/issues/27424
            tc.extra_ldflags.append("-headerpad_max_install_names")

        self._create_targets(tc.cflags, tc.cxxflags, tc.defines, tc.ldflags)
        tc.generate(env)

    def _create_targets(self, cflags, cxxflags, defines, ldflags):
        config_template = textwrap.dedent("""\
            {targets} = (
                "{target}" => {{
                    inherit_from => {ancestor},
                    cflags => add("{cflags}"),
                    cxxflags => add("{cxxflags}"),
                    {defines}
                    lflags => add("{lflags}"),
                    {shared_target}
                    {shared_cflag}
                    {shared_extension}
                    {perlasm_scheme}
                }},
            );
        """)

        perlasm_scheme = ""
        if self._perlasm_scheme:
            perlasm_scheme = f'perlasm_scheme => "{self._perlasm_scheme}",'

        defines = '", "'.join(defines)
        defines = 'defines => add("%s"),' % defines if defines else ""
        targets = "my %targets"

        if self._asm_target:
            ancestor = f'[ "{self._ancestor_target}", asm("{self._asm_target}") ]'
        else:
            ancestor = f'[ "{self._ancestor_target}" ]'
        shared_cflag = ""
        shared_extension = ""
        shared_target = ""
        if self.settings.os == "Neutrino":
            if self.options.shared:
                shared_extension = r'shared_extension => ".so.\$(SHLIB_VERSION_NUMBER)",'
                shared_target = 'shared_target  => "gnu-shared",'
            if self.options.get_safe("fPIC", True):
                shared_cflag = 'shared_cflag => "-fPIC",'

        if self.settings.os == "Android":
            shared_extension = r'shared_extension => "_3.so",'

        if self.settings.os in ["iOS", "tvOS", "watchOS"] and self.conf.get("tools.apple:enable_bitcode", check_type=bool):
            cflags.append("-fembed-bitcode")
            cxxflags.append("-fembed-bitcode")

        config = config_template.format(
            targets=targets,
            target=self._target,
            ancestor=ancestor,
            cflags=" ".join(cflags),
            cxxflags=" ".join(cxxflags),
            defines=defines,
            perlasm_scheme=perlasm_scheme,
            shared_target=shared_target,
            shared_extension=shared_extension,
            shared_cflag=shared_cflag,
            lflags=" ".join(ldflags)
        )
        self.output.info(f"using target: {self._target} -> {self._ancestor_target}")
        self.output.info(config)

        save(self, os.path.join(self.source_folder, "Configurations", "20-conan.conf"), config)

    def _run_make(self, targets=None, parallel=True, install=False):
        command = [self._make_program]
        if install:
            command.append(f"DESTDIR={self._adjust_path(self.package_folder)}")
        if targets:
            command.extend(targets)
        if self._make_program in ["make", "jom"]:
            command.append(f"-j{build_jobs(self)}" if parallel else "-j1")
        self.run(" ".join(command), env="conanbuild")

    @property
    def _perl(self):
        if self._use_nmake:
            return self.dependencies.build["strawberryperl"].conf_info.get("user.strawberryperl:perl", check_type=str)
        return "perl"

    def _make(self):
        with chdir(self, self.source_folder):
            args = " ".join(self._configure_args)

            if self._use_nmake:
                self._replace_runtime_in_file(os.path.join("Configurations", "10-main.conf"))

            self.run(f"{self._perl} ./Configure {args}", env="conanbuild")
            if self._use_nmake:
                # When `--prefix=/`, the scripts derive `\` without escaping, which
                # causes issues on Windows
                replace_in_file(self, "Makefile", "INSTALLTOP_dir=\\", "INSTALLTOP_dir=\\\\")
                if Version(self.version) >= "3.3.0":
                    # replace backslashes in paths with forward slashes
                    mkinstallvars_pl = os.path.join(self.source_folder, "util", "mkinstallvars.pl")
                    if Version(self.version) >= "3.3.2":
                        replace_in_file(self, mkinstallvars_pl, "push @{$values{$k}}, $v;", """$v =~ s|\\\\|/|g; push @{$values{$k}}, $v;""")
                        replace_in_file(self, mkinstallvars_pl, "$values{$k} = $v;", """$v->[0] =~ s|\\\\|/|g; $values{$k} = $v;""")
                    else:
                        replace_in_file(self, mkinstallvars_pl, "$ENV{$k} = $v;", """$v =~ s|\\\\|/|g; $ENV{$k} = $v;""")
            self._run_make()

    def _make_install(self):
        with chdir(self, self.source_folder):
            self._run_make(targets=["install_sw"], parallel=False, install=True)

    def build(self):
        self._make()
        configdata_pm = self._adjust_path(os.path.join(self.source_folder, "configdata.pm"))
        self.run(f"{self._perl} {configdata_pm} --dump")

    @property
    def _make_program(self):
        use_jom = self._use_nmake and self.conf.get("user.openssl:windows_use_jom", False)
        if self._use_nmake:
            return "jom" if use_jom else "nmake"
        else:
            return "make"

    def _replace_runtime_in_file(self, filename):
        runtime = msvc_runtime_flag(self)
        for e in ["MDd", "MD", "MT"]:
            replace_in_file(self, filename, f"/{e} ", f"/{runtime} ")
            replace_in_file(self, filename, f"/{e}\"", f"/{runtime}\"")

    def package(self):
        copy(self, "*LICENSE*", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        self._make_install()
        if is_apple_os(self):
            fix_apple_shared_install_name(self)

        rm(self, "*.pdb", self.package_folder, "lib")
        if self.options.shared:
            libdir = os.path.join(self.package_folder, "lib")
            for file in os.listdir(libdir):
                if self._is_mingw and file.endswith(".dll.a"):
                    continue
                if file.endswith(".a"):
                    os.unlink(os.path.join(libdir, file))

        if not self.options.no_fips:
            provdir = os.path.join(self.source_folder, "providers")
            modules_dir = os.path.join(self.package_folder, "lib", "ossl-modules")
            if self.settings.os == "Macos":
                copy(self, "fips.dylib", src=provdir, dst=modules_dir)
            elif self.settings.os == "Windows":
                copy(self, "fips.dll", src=provdir, dst=modules_dir)
            else:
                copy(self, "fips.so", src=provdir, dst=modules_dir)

        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

        self._create_cmake_module_variables(
            os.path.join(self.package_folder, self._module_file_rel_path)
        )

    def _create_cmake_module_variables(self, module_file):
        content = textwrap.dedent("""\
            set(OPENSSL_FOUND TRUE)
            if(DEFINED OpenSSL_INCLUDE_DIR)
                set(OPENSSL_INCLUDE_DIR ${OpenSSL_INCLUDE_DIR})
            endif()
            if(DEFINED OpenSSL_Crypto_LIBS)
                set(OPENSSL_CRYPTO_LIBRARY ${OpenSSL_Crypto_LIBS})
                set(OPENSSL_CRYPTO_LIBRARIES ${OpenSSL_Crypto_LIBS}
                                             ${OpenSSL_Crypto_DEPENDENCIES}
                                             ${OpenSSL_Crypto_FRAMEWORKS}
                                             ${OpenSSL_Crypto_SYSTEM_LIBS})
            elseif(DEFINED openssl_OpenSSL_Crypto_LIBS_%(config)s)
                set(OPENSSL_CRYPTO_LIBRARY ${openssl_OpenSSL_Crypto_LIBS_%(config)s})
                set(OPENSSL_CRYPTO_LIBRARIES ${openssl_OpenSSL_Crypto_LIBS_%(config)s}
                                             ${openssl_OpenSSL_Crypto_DEPENDENCIES_%(config)s}
                                             ${openssl_OpenSSL_Crypto_FRAMEWORKS_%(config)s}
                                             ${openssl_OpenSSL_Crypto_SYSTEM_LIBS_%(config)s})
            endif()
            if(DEFINED OpenSSL_SSL_LIBS)
                set(OPENSSL_SSL_LIBRARY ${OpenSSL_SSL_LIBS})
                set(OPENSSL_SSL_LIBRARIES ${OpenSSL_SSL_LIBS}
                                          ${OpenSSL_SSL_DEPENDENCIES}
                                          ${OpenSSL_SSL_FRAMEWORKS}
                                          ${OpenSSL_SSL_SYSTEM_LIBS})
            elseif(DEFINED openssl_OpenSSL_SSL_LIBS_%(config)s)
                set(OPENSSL_SSL_LIBRARY ${openssl_OpenSSL_SSL_LIBS_%(config)s})
                set(OPENSSL_SSL_LIBRARIES ${openssl_OpenSSL_SSL_LIBS_%(config)s}
                                          ${openssl_OpenSSL_SSL_DEPENDENCIES_%(config)s}
                                          ${openssl_OpenSSL_SSL_FRAMEWORKS_%(config)s}
                                          ${openssl_OpenSSL_SSL_SYSTEM_LIBS_%(config)s})
            endif()
            if(DEFINED OpenSSL_LIBRARIES)
                set(OPENSSL_LIBRARIES ${OpenSSL_LIBRARIES})
            endif()
            if(DEFINED OpenSSL_VERSION)
                set(OPENSSL_VERSION ${OpenSSL_VERSION})
            endif()
        """% {"config":str(self.settings.build_type).upper()})
        save(self, module_file, content)

    @property
    def _module_subfolder(self):
        return os.path.join("lib", "cmake")

    @property
    def _module_file_rel_path(self):
        return os.path.join(self._module_subfolder,
                            f"conan-official-{self.name}-variables.cmake")

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "OpenSSL")
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("pkg_config_name", "openssl")
        self.cpp_info.set_property("cmake_build_modules", [self._module_file_rel_path])
        self.cpp_info.components["ssl"].builddirs.append(self._module_subfolder)
        self.cpp_info.components["ssl"].set_property("cmake_build_modules", [self._module_file_rel_path])
        self.cpp_info.components["crypto"].builddirs.append(self._module_subfolder)
        self.cpp_info.components["crypto"].set_property("cmake_build_modules", [self._module_file_rel_path])

        if self._use_nmake:
            self.cpp_info.components["ssl"].libs = ["libssl"]
            self.cpp_info.components["crypto"].libs = ["libcrypto"]
        else:
            self.cpp_info.components["ssl"].libs = ["ssl"]
            self.cpp_info.components["crypto"].libs = ["crypto"]

        self.cpp_info.components["ssl"].requires = ["crypto"]

        if not self.options.no_zlib:
            self.cpp_info.components["crypto"].requires.append("zlib::zlib")

        if self.settings.os == "Windows":
            self.cpp_info.components["crypto"].system_libs.extend(["crypt32", "ws2_32", "advapi32", "user32", "bcrypt"])
        elif self.settings.os == "Linux":
            self.cpp_info.components["crypto"].system_libs.extend(["dl", "rt"])
            self.cpp_info.components["ssl"].system_libs.append("dl")
            if not self.options.no_threads:
                self.cpp_info.components["crypto"].system_libs.append("pthread")
                self.cpp_info.components["ssl"].system_libs.append("pthread")
        elif self.settings.os == "Neutrino":
            self.cpp_info.components["crypto"].system_libs.append("atomic")
            self.cpp_info.components["ssl"].system_libs.append("atomic")
            self.cpp_info.components["crypto"].system_libs.append("socket")
            self.cpp_info.components["ssl"].system_libs.append("socket")

        self.cpp_info.components["crypto"].set_property("cmake_target_name", "OpenSSL::Crypto")
        self.cpp_info.components["crypto"].set_property("pkg_config_name", "libcrypto")
        self.cpp_info.components["ssl"].set_property("cmake_target_name", "OpenSSL::SSL")
        self.cpp_info.components["ssl"].set_property("pkg_config_name", "libssl")

        openssl_modules_dir = os.path.join(self.package_folder, "lib", "ossl-modules")
        self.runenv_info.define_path("OPENSSL_MODULES", openssl_modules_dir)