#!/usr/bin/env python
"""
Builds this project from the ground up, run unit tests and deploys it to Azure or Heroku, well, not just yet :)
"""
import os
import re
import sys
import glob
import json
import base64
import shutil
import zipfile
import tarfile
import argparse
import threading
import subprocess
import multiprocessing

try:
    from utils import *
except:
    from .utils import *

try:  # For Python 3.0 and later
    from urllib.request import urlopen
    from urllib.request import Request
except ImportError:   # Fall back to Python 2's urllib2
    from urllib2 import urlopen
    from urllib2 import Request


class ShellRunner(object):
    def __init__(self, arch_name='', env=None):
        self._env = os.environ.copy()
        if env:
            self._env.update(env)

        self._extra_paths = []
        self._arch_name = arch_name
        self._is_emscripten = arch_name == 'wasm' or arch_name == 'web'

        if arch_name == 'x64':
            if is_windows():
                self._detect_vs_version()

        # Add tools like ninja and swig to the current PATH
        this_dir = os.path.dirname(os.path.realpath(__file__))
        tools_dir = make_path(this_dir, 'tools', platform_name())
        self.add_system_path(tools_dir)

    def add_system_path(self, new_path, at_end=True):
        curr_path_str = self._env['PATH']
        path_elmts = set(curr_path_str.split(os.pathsep))
        if new_path in path_elmts:
            return
        if at_end:
            self._env['PATH'] = curr_path_str + os.pathsep + new_path
        else:
            self._env['PATH'] = new_path + os.pathsep + curr_path_str

    def set_env_var(self, var_name, var_value):
        assert isinstance(var_name, str), 'var_name should be a string'
        assert isinstance(
            var_value, str) or var_value is None, 'var_value should be a string or None'
        self._env[var_name] = var_value

    def get_env_var(self, var_name):
        return self._env.get(var_name, '')

    def get_env(self):
        return self._env

    def run_cmd(self, cmd_args, cmd_print=True, cwd=None, input=None):
        """
        Runs a shell command
        """
        if isinstance(cmd_args, str):
            cmd_args = cmd_args.split()
        cmd_all = []
        if is_windows() and self._arch_name in ['x64']:
            cmd_all = [self._vcvarsbat, 'x86_amd64', '&&']
        cmd_all = cmd_all + cmd_args

        if cmd_print:
            print(' '.join(cmd_args))

        use_shell = os.name == 'nt'
        p = subprocess.Popen(cmd_all, env=self._env, cwd=cwd, shell=use_shell,
                             stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
        if input:
            p.communicate(input=input)
        else:
            p.wait()
        if p.returncode != 0:
            print('Command "%s" exited with code %d' % (' '.join(cmd_args), p.returncode))
            sys.exit(p.returncode)

    def _detect_vs_version(self):
        """
        Detects the first available version of Visual Studio
        """
        vc_releases = [
            ('Visual Studio 16 2019',
             r'C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat'),
            ('Visual Studio 16 2019',
             r'C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat'),
            ('Visual Studio 16 2019',
             r'C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat'),
            ('Visual Studio 15 2017',
             r'C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat'),
            ('Visual Studio 15 2017',
             r'C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat'),
            ('Visual Studio 15 2017',
             r'C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat'),
            ('Visual Studio 14 2015', r'C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat')]
        for (vsgenerator, vcvarsbat) in vc_releases:
            if os.path.exists(vcvarsbat):
                self._vcvarsbat = vcvarsbat
                self._vc_cmake_gen = vsgenerator
                if "64" in self._arch_name:
                    self._vc_cmake_gen += ' Win64'
                break

    def get_vc_cmake_generator(self):
        return self._vc_cmake_gen


class Builder(object):
    """
    Class that holds the whole building process
    """

    def repo_path(self, rel_path=''):
        if not rel_path:
            return self._root_dir
        return make_path(self._root_dir, rel_path).replace('\\', '/')

    def webapp_path(self, rel_path=''):
        """
        Returns the absolute path of the webapp directory (depending on whether we using NodeJs or .Net Core server)
        """
        if not rel_path:
            return make_path(self._root_dir, 'webapp')
        return make_path(self._root_dir, 'webapp', rel_path)

    def build_name(self):
        if self._arch_name == 'wasm':
            return 'emscripten_' + self._build_config
        return platform_name() + '_' + self._build_config + '_' + self._arch_name

    def build_dir_name(self, prefix):
        """
        Returns a name for a build directory based on the build configuration
        """
        return make_path(prefix, 'build_' + self.build_name())

    def run_cmd(self, cmd_args, cmd_print=True, cwd=None, input=None):
        self._shell.run_cmd(cmd_args, cmd_print=cmd_print,
                            cwd=cwd, input=input)

    def set_startup_vs_prj(self, project_name):
        """
        Rearranges the projects so that the specified project is the first
        therefore is the startup project within Visual Studio
        """
        solution_file = glob.glob(self._build_dir + '/*.sln')[0]
        sln_lines = read_file(solution_file).splitlines()
        lnum = 0
        lin_prj_beg = 0
        lin_prj_end = 0
        for line in sln_lines:
            if project_name in line:
                lin_prj_beg = lnum
            if lin_prj_beg > 0 and line.endswith('EndProject'):
                lin_prj_end = lnum
                break
            lnum = lnum + 1
        prj_lines = sln_lines[:2] + sln_lines[lin_prj_beg:lin_prj_end + 1] \
            + sln_lines[2:lin_prj_beg] + sln_lines[lin_prj_end + 1:]
        write_file(solution_file, '\n'.join(item for item in prj_lines))

    def get_third_party_lib_dir(self, prefix):
        """
        Get the directory where a third party library with the specified prefix
        name was extracted, if any
        """
        third_party_dirs = next(os.walk(self._third_party_dir))[1]
        candidate = None
        for lib_dir in sorted(third_party_dirs):
            if prefix in lib_dir:
                candidate = make_path(self._third_party_dir, lib_dir)
        return candidate

    def clean_thirdparty_if_needed(self, libname, installed_dir, extract_dir):
        if libname in self._clean_targets:
            if os.path.isdir(installed_dir):
                shutil.rmtree(installed_dir)
            if extract_dir and os.path.isdir(extract_dir):
                shutil.rmtree(extract_dir)

    def build_third_party_lib(self, tpl):
        """
        Downloads and builds a third party library
        """
        architectures = tpl.get('architectures', [])
        if architectures and self._arch_name not in architectures:
            return

        name = tpl.get('name')
        src_url = tpl.get('srcUrl')
        download_pkg = tpl.get('downloadPkg')
        extract_dir = tpl.get('extractDir')
        header_only = tpl.get('headerOnly', False)

        # Check if configuration has changed
        last_build_config_file = make_path(self._third_party_install_dir, name + '.json')
        this_build_config = json.dumps(tpl)
        last_build_config = read_file(last_build_config_file)
        if not last_build_config:
            last_build_config = this_build_config

        extract_dir = make_path(self._third_party_dir, extract_dir)
        rebuild = last_build_config != this_build_config or name in self._clean_targets

        for ic in tpl.get('installChecks', []):
            ic_path = make_path(self._third_party_install_dir, ic)
            if os.path.exists(ic_path):
                if rebuild:
                    if os.path.isfile(ic_path):
                        os.remove(ic_path)
                    else:
                        shutil.rmtree(ic_path)
                else:
                    # Library is built and installed, nothing to do
                    return

        if rebuild and os.path.isdir(extract_dir):
            shutil.rmtree(extract_dir)

        # Download library sources if not done yet
        src_pkg = self.download_third_party_lib(src_url, download_pkg)

        if not os.path.isdir(extract_dir):
            # Extract the source files
            self.extract_third_party_lib(src_pkg)

        # Some libraries need some hacks in the CMakeLists.txt before building
        self.hack_third_party_cmake(name, extract_dir)

        # If header only we are done
        if header_only:
            write_file(last_build_config_file, this_build_config)
            return

        ced = tpl.get('cmakeExtraDefinitions', {}).copy()
        cmake_extra_defs = ced.get('common', [])
        desktop = self._arch_name != 'wasm'
        if desktop:
            cmake_extra_defs += ced.get('desktop', [])
            p_name = platform_name()
            cmake_extra_defs += ced.get(p_name, [])
        else:
            cmake_extra_defs += ced.get('wasm', [])

        cmake_extra_defs = [f'-D{flag}' if not flag.startswith('-D') else flag for flag in cmake_extra_defs]

        # Clean and create the build directory
        build_dir = self.build_dir_name(extract_dir)
        if os.path.exists(build_dir):  # Remove the build directory
            shutil.rmtree(build_dir)
        if not os.path.exists(build_dir):  # Create the build directory
            os.mkdir(build_dir)
        self.build_cmake_lib(extract_dir, cmake_extra_defs, ['install'], True)

        # Save the current build configuration
        write_file(last_build_config_file, this_build_config)

    def hack_third_party_cmake(self, lib_name, lib_extract_dir):
        if lib_name == 'dlib':
            # Hacks to compile dlib
            dlib_cmakelists_path = make_path(lib_extract_dir, 'dlib/CMakeLists.txt')
            if is_windows():
                # Hack cmakelists.txt to get option -DDLIB_FORCE_MSVC_STATIC_RUNTIME to work
                hack_line = 'include(cmake_utils/tell_visual_studio_to_use_static_runtime.cmake)'
                content = read_file(dlib_cmakelists_path)
                if hack_line not in content:
                    content = hack_line + '\n' + content
                write_file(dlib_cmakelists_path, content)
            else:
                content = read_file(dlib_cmakelists_path)
                content = content.replace('cmake_minimum_required(VERSION 2.8.12)',
                                          'cmake_minimum_required(VERSION 3.1.0)')
                write_file(dlib_cmakelists_path, content)

    def get_filename_from_url(self, url):
        """
        Extracts the file name from a given URL
        """
        lib_filename = url.split('/')[-1].split('#')[0].split('?')[0]
        lib_filepath = make_path(self._third_party_dir, lib_filename)
        return lib_filepath

    def download_third_party_lib(self, url, package_name=None):
        """
        Download a third party dependency from the internet if is not available offline
        """
        if not package_name:
            lib_filepath = self.get_filename_from_url(url)
        else:
            lib_filepath = make_path(self._third_party_dir, package_name)
        if not os.path.exists(lib_filepath):
            print('Downloading %s to "%s" please wait ...' % (url, lib_filepath))
            req = Request(url, headers={"User-Agent": "Chrome"})
            lib_file = urlopen(req)
            write_file(lib_filepath, lib_file.read(), 'wb')
        return lib_filepath

    def get_third_party_lib_version(self, url):
        a = re.compile(r'\d+\.\d+(:?\.\d+)?')
        m = a.search(url)
        return m.group(0) if m else None

    def extract_third_party_lib(self, lib_src_pkg, extract_dir=None):
        """
        Extracts a third party lib package source file into a directory
        """
        if not extract_dir:
            extract_dir = self._third_party_dir
        print('Extracting third party library "%s" into "%s" ... please wait ...' % (lib_src_pkg, extract_dir))
        if 'zip' in lib_src_pkg:
            zip_handle = zipfile.ZipFile(lib_src_pkg)
            for item in zip_handle.namelist():
                zip_handle.extract(item, extract_dir)
            zip_handle.close()
        else:  # Assume tar archive (tgz, tar.bz2, tar.gz)
            tar = tarfile.open(lib_src_pkg, 'r')
            for item in tar:
                tar.extract(item, self._third_party_dir)
            tar.close()

    def build_cmake_lib(self, cmakelists_path, extra_definitions, targets, is_3rd_party, clean_build=False):
        """
        Builds a library using cmake
        """
        build_dir = self.build_dir_name(cmakelists_path)
        # Clean and create the build directory
        # Remove the build directory
        if clean_build and os.path.exists(build_dir):
            shutil.rmtree(build_dir)
        if not os.path.exists(build_dir):  # Create the build directory
            os.mkdir(build_dir)

        if 'install' in targets:
            install_dir = self._third_party_install_dir if is_3rd_party else self._install_dir
            extra_definitions.append('-DCMAKE_INSTALL_PREFIX=' + install_dir)

        if self._arch_name == 'wasm':
            emscripten_path = make_path(self._third_party_dir, 'emsdk/' + \
                                        self._emsdk_backend + '/emscripten')
            if not os.path.isdir(emscripten_path):
                raise Exception(emscripten_path + ' does NOT exist!')

            cmake_module_path = make_path(emscripten_path, 'cmake')
            cmake_toolchain = make_path(cmake_module_path, 'Modules', 'Platform', 'Emscripten.cmake')

            s_conf = self.get_emscripten_config(True)

            cxx_flags_cfg = self.config['emsdk']['cxxFlags']
            cxx_flags = cxx_flags_cfg.get('common', '') + ' ' + cxx_flags_cfg.get(self._build_config, '')
            cxx_flags = s_conf + ' ' + cxx_flags

            extra_definitions += [
                '-DEMSCRIPTEN=1',
                '-DCMAKE_TOOLCHAIN_FILE=' + cmake_toolchain.replace('\\', '/'),
                '-DCMAKE_MAKE_PROGRAM=ninja',
                '-DCMAKE_MODULE_PATH=' + cmake_module_path.replace('\\', '/'),
                '-DCMAKE_CXX_FLAGS=' + cxx_flags + '',
             #   '-DCMAKE_EXE_LINKER_FLAGS=' + cxx_flags + ''
            ]

        if self._arch_name != 'wasm':
            c_compiler = self._shell.get_env_var('CC')
            if c_compiler:
                extra_definitions.append('-DCMAKE_C_COMPILER=' + c_compiler)
            cxx_compiler = self._shell.get_env_var('CXX')
            if cxx_compiler:
                extra_definitions.append('-DCMAKE_CXX_COMPILER=' + cxx_compiler)

        # Define CMake generator and make command
        os.chdir(build_dir)
        cmake_cmd = ['cmake', '-G', 'Ninja',
                     '-DCMAKE_BUILD_TYPE=' + self._build_config] + extra_definitions + [cmakelists_path.replace('\\', '/')]

        # Run CMake and Make
        self.run_cmd(cmake_cmd)
        self.run_cmd('ninja')
        for target in targets:
            self.run_cmd(['ninja', target])
        os.chdir(self._root_dir)

    def setup_android(self):
        # Download SDK tools if not present and extract it to
        p_name = platform_name()
        ANDROID_SDK_TOOLS = self.config["android"]["sdk"][p_name]
        ANDROID_GRADLE = self.config["android"]["gradle"]
        android_sdk_tools_pkg = self.download_third_party_lib(ANDROID_SDK_TOOLS)

        # Extract the SDK if not done already
        android_sdk_tools_dirname = os.path.splitext(os.path.basename(android_sdk_tools_pkg))[0]
        android_sdk_tools_dir = self.get_third_party_lib_dir(android_sdk_tools_dirname)
        if android_sdk_tools_dir is None:
            android_sdk_tools_dir = make_path(self._third_party_dir, android_sdk_tools_dirname)
            self.extract_third_party_lib(android_sdk_tools_pkg, android_sdk_tools_dir)

        # Download gradle
        gradle_pkg = self.download_third_party_lib(ANDROID_GRADLE)
        gradle_pkg_dir = self.get_third_party_lib_dir('gradle')
        if gradle_pkg_dir is None:
            self.extract_third_party_lib(gradle_pkg)
            gradle_pkg_dir = self.get_third_party_lib_dir('gradle')

        # # Download Android NDK if not present
        # android_ndk_pkg = self.download_third_party_lib(ANDROID_NDK)

        # # Extract the NDK if not done already
        # android_ndk_dir = self.get_third_party_lib_dir('android-ndk')
        # if android_ndk_dir is None:
        #     self.extract_third_party_lib(android_ndk_pkg)

        # Set up environment
        android_user_dir = make_path(os.path.expanduser("~"), '.android')
        if not os.path.exists(android_user_dir):
            os.mkdir(android_user_dir)
        repos_cfg = make_path(android_user_dir, 'repositories.cfg')
        if not os.path.exists(repos_cfg):
            write_file(repos_cfg, '')

        # self._shell.set_env_var('JAVA_HOME', '/usr/lib/jvm/java-8-oracle')
        self._shell.set_env_var('ANDROID_HOME', android_sdk_tools_dir)
        self._shell.set_env_var('ANDROID_SDK_ROOT', android_sdk_tools_dir)
        # self._shell.set_env_var('ANDROID_NDK_HOME', android_ndk_dir)
        # self._shell.set_env_var('JAVA_OPTS', '-XX:+IgnoreUnrecognizedVMOptions --add-modules java.se.ee')
        # self._shell.set_env_var('JAVA_OPTS', '')

        bin_tools = os.path.normpath(os.path.join(android_sdk_tools_dir, 'tools/bin'))
        self._shell.add_system_path(bin_tools)
        self._shell.add_system_path(os.path.normpath(os.path.join(android_sdk_tools_dir, 'tools')))
        self._shell.add_system_path(os.path.normpath(os.path.join(gradle_pkg_dir, 'bin')))
        # self._shell.add_system_path(os.path.normpath(android_ndk_dir))
        self._shell.add_system_path(os.path.normpath(
            make_path(self._shell.get_env_var('JAVA_HOME'), '/jre/bin')))

        # print(self._shell._env)
        if os.name == 'posix':
            self.run_cmd('chmod -R +x {}'.format(bin_tools))
            # self.run_cmd('chmod -R +x {}'.format(android_ndk_dir))
            self.run_cmd('chmod -R +x {}/bin'.format(gradle_pkg_dir))
        self.run_cmd('yes | sdkmanager --licenses')
        # self.run_cmd('sdkmanager "platform-tools" "platforms;android-25"', input='y')

    def get_emscripten_config(self, as_compiler_args):
        config = self.config['emsdk'].get('config')
        if as_compiler_args:
            return ' '.join('-s ' + k + '=' + str(config[k]) for k in config)
        else:
            return config

    def setup_emscripten(self):
        if which('emsdk'):
            return  # we already have emscripten in path
        emsdk_dir = make_path(self._third_party_dir, 'emsdk')
        emsdk_cmd = 'emsdk.bat' if is_windows() else './emsdk'

        emsdk_version_number = str(self.emsdk_version_number)
        emsdk_version_name = emsdk_version_number
        # emsdk_version_name = 'sdk-' + emsdk_version_number + '-64bit'
        # if 'upstream' in self._emsdk_backend:
        #     emsdk_version_name += '-' + self._emsdk_backend
        emsdk_backend_dir = make_path(emsdk_dir, self._emsdk_backend)
        if not os.path.exists(emsdk_dir):
            os.chdir(self._third_party_dir)
            self.run_cmd('git clone https://github.com/emscripten-core/emsdk.git emsdk')
        if not os.path.exists(emsdk_backend_dir):
            os.chdir(emsdk_dir)
            self.run_cmd(emsdk_cmd + ' install ' + emsdk_version_name)
        os.chdir(emsdk_dir)
        self.run_cmd(emsdk_cmd + ' activate ' + emsdk_version_name)
        process = subprocess.Popen(['python', 'emsdk.py', 'construct_env'], stdout=subprocess.PIPE)
        (output, _) = process.communicate()
        exit_code = process.wait()
        if exit_code != 0:
            exit(exit_code)
        path_re = re.compile(r'PATH \+= (.*)')
        envvar_re = re.compile(r'([A-Za-z_]+) = (.*)')
        if not isinstance(output, str):
            output = output.decode("utf-8")
        for line in output.splitlines():
            m = path_re.search(line)
            if m:
                path = m.group(1)
                path = path.replace('\\', '/')
                self._shell.add_system_path(path, at_end=True)
                continue
            m = envvar_re.search(line)
            if m:
                var_name = m.group(1)
                var_value = m.group(2).replace('\\', '/')
                self._shell.set_env_var(var_name, var_value)
                continue
        os.chdir(self._root_dir)
        self._shell.set_env_var('CC', 'emcc')
        self._shell.set_env_var('CXX', 'em++')

        # Configure settings.js as for some reason flags passed in CMAKE_CXX_FLAGS do not get really used
        config = self.get_emscripten_config(False)

        settings_file = None
        for p in ['fastcomp/emscripten', 'upstream/emscripten']:
            candidate = make_path(emsdk_dir, p, 'src', 'settings.js')
            if os.path.isfile(candidate):
                settings_file = candidate
                break

        if not settings_file:
            self.run_cmd(['tree', emsdk_dir])
            raise Exception('Unable to find emscripten\'s settings file')

        content = read_file(settings_file)
        new_content = content
        for name in config:
            new_content = re.sub(r'(var ' + name + r'\s?=\s?)([A-z0-9\[\]"]+);',
                                 r'\g<1>' + str(config[name]) + r';', new_content)
        if new_content != content:
            write_file(settings_file, new_content)

    def build_cpp_code(self):
        """
        Builds the C++ libppp project from sources
        """
        # self.run_cmd(
        #     'swig -c++ -python -Ilibppp/include -outdir libppp/python -o libppp/swig/libppp_python_wrap.cxx libppp/swig/libppp.i')

        # Clean build and install directory first if needed
        if 'ppp' in self._clean_targets:
            for path in [self._build_dir, self._install_dir]:
                if os.path.isdir(path):
                    shutil.rmtree(self._build_dir)

        if not os.path.exists(self._build_dir):
            # Create the build directory if doesn't exist
            os.mkdir(self._build_dir)

        # Change directory to build directory
        os.chdir(self._build_dir)

        cmake_extra_defs = [
            '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'
        ]

        if self._gen_vs_sln:
            # Generating visual studio solution
            cmake_generator = self._shell.get_vc_cmake_generator()
            cmake_args = ['cmake',
                          '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                          '-DCMAKE_INSTALL_PREFIX=' + self._install_dir,
                          '-DCMAKE_PREFIX_PATH=' + self._install_dir,
                          '-DCMAKE_BUILD_TYPE=' + self._build_config,
                          '-G', cmake_generator, '..']
            self.run_cmd(cmake_args)
        else:
            targets = ['install'] if self._arch_name != 'wasm' else []
            self.build_cmake_lib('..', cmake_extra_defs, targets, False)
            # Run unit tests for C++ code
            if self._arch_name in ['x64', 'x86'] and self._run_tests:
                os.chdir(self._install_dir)
                test_exe = r'.\yaiat_test.exe' if is_windows() else './yaiat_test'
                self.run_cmd([test_exe, '--gtest_output=xml:../tests.xml'])

        os.chdir(self._root_dir)
        if self._arch_name == 'wasm':
            # Copy build artifacts
            wasm_files = glob.glob(self._build_dir + "/**/*.wasm*", recursive=True)
            js_files = glob.glob(self._build_dir + "/**/*.js", recursive=True)
            for build_artifact in wasm_files + js_files:
                shutil.copy(build_artifact, self.repo_path('webapp/src/assets'))

            # # Copy resources (models, etc.)
            # root_src_dir = self.repo_path('libppp/share')
            # root_dst_dir = self.repo_path('webapp/src/assets/')
            # for src_dir, _, files in os.walk(root_src_dir):
            #     dst_dir = src_dir.replace(root_src_dir, root_dst_dir, 1)
            #     if not os.path.exists(dst_dir):
            #         os.makedirs(dst_dir)
            #     for file_ in files:
            #         src_file = make_path(src_dir, file_)
            #         dst_file = make_path(dst_dir, file_)
            #         if os.path.exists(dst_file):
            #             os.remove(dst_file)
            #         shutil.copy(src_file, dst_dir)

    def build_android(self):
        """
        Builds android app
        """
        self.run_cmd('npx cap copy', cwd='webapp')
        self.run_cmd('gradle build --stacktrace', cwd='webapp/android')

    def build_webapp(self):
        """
        Builds and test the web application by running shell commands
        """
        # Build the web app
        os.chdir(self.webapp_path())
        self.run_cmd('npm run gen-app-info')
        if self._run_tests:
            self.run_cmd('npx ng test --browsers=ChromeHeadless --watch=false')
        self.run_cmd('npx ng build --prod')
        os.chdir(self._root_dir)

    def setup_webapp(self):
        if not which('npx'):
            self.run_cmd('npm install npx -g')

        # Build the web app
        if not self._no_npm:
            os.chdir(self.webapp_path())
            self.run_cmd('npm install --no-optional')
            os.chdir(self._root_dir)

    def load_build_config(self):
        # Configuration
        this_dir = os.path.dirname(os.path.realpath(__file__))
        config_file = make_path(this_dir, 'build.json')
        self.config = read_json(config_file)

        self.emsdk_version_number = self.config['emsdk']['version']
        self.emsdk_version_name = 'sdk-' + self.emsdk_version_number + '-64bit'
        self._emsdk_backend = 'fastcomp' if 'fastcomp' in self.emsdk_version_number else 'upstream'

    def parse_arguments(self):
        """
        Parses command line arguments
        """
        parser = argparse.ArgumentParser(description='Builds this application and it dependencies')
        parser.add_argument('--arch_names', '-a', required=False, choices=['x64', 'wasm', 'web', 'android', 'all'],
                            help='Platform architectures to build', default=['x64'], nargs='+')
        parser.add_argument('--build_config', '-b', required=False, choices=['debug', 'release'],
                            help='Build configuration type', default='release')
        parser.add_argument('--clean_targets', '-c', nargs='+', help='Clean specified targets', default=[])
        parser.add_argument('--test', '-t', help='Runs available unit tests', action="store_true")
        parser.add_argument('--skip_install', help='Skips installation', action="store_true")
        parser.add_argument('--gen_vs_sln', help='Generates Visual Studio solution and projects',
                            action="store_true")
        parser.add_argument('--no_npm', help='Skips installing npm packages. Use only on developer workflow',
                            action="store_true")

        args = parser.parse_args()

        arch_names = args.arch_names
        if 'all' in arch_names:
            arch_names == ['x64', 'wasm', 'web']  # Add android

        build_order = {'x64': 1, 'x86': 1, 'wasm': 1, 'web': 2, 'android': 3}
        self._arch_names = sorted(arch_names, key=lambda a: build_order[a])
        self._clean_targets = args.clean_targets
        self._build_config = args.build_config
        self._gen_vs_sln = args.gen_vs_sln
        self._run_tests = args.test
        self._run_install = not args.skip_install
        self._no_npm = args.no_npm

    def clean_all_if_needed(self):
        if 'all' in self._clean_targets:
            if os.path.isdir(self._third_party_install_dir):
                shutil.rmtree(self._third_party_install_dir)
            if os.path.isdir(self._build_dir):
                shutil.rmtree(self._build_dir)

    def __init__(self):
        self._root_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

        # Load 3rd party config
        self.load_build_config()

        # Detect OS version
        self.parse_arguments()

        # Extract testing dataset
        self._shell = ShellRunner()

        for arch in self._arch_names:
            self._arch_name = arch

            # directory suffix for the build and release
            self._build_dir = make_path(self._root_dir, 'build_' + self.build_name())
            self._install_dir = make_path(self._root_dir, 'install_' + self.build_name())
            self._third_party_dir = make_path(self._root_dir, 'thirdparty')
            self._third_party_install_dir = make_path(self._third_party_dir, 'install_' + self.build_name())

            env = self.config.get(arch, {}).get(platform_name(), {})
            shell = ShellRunner(arch, env)
            shell.set_env_var('INSTALL_DIR', self._install_dir)
            self._shell = shell

            # Clean all if requested
            self.clean_all_if_needed()

            # Setup Emscripten tools
            if arch == 'wasm':
                self.setup_emscripten()

            # Create install directory if it doesn't exist
            if not os.path.exists(self._install_dir):
                os.mkdir(self._install_dir)

            # Setup tools and build web app
            if arch == 'web':
                self.setup_webapp()
                self.build_webapp()

            if arch in ['x64', 'wasm']:
                # Build Third Party Libs
                for tpl in self.config.get('thirdPartyLibs', []):
                    self.build_third_party_lib(tpl)
                # Build libppp
                self.build_cpp_code()

            if arch == 'android':
                self.setup_android()
                self.build_android()


BUILDER = Builder()
