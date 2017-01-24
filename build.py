import os
import sys
import glob
import shutil
import argparse
import subprocess
import zipfile
import tarfile
import multiprocessing

# Configuration
gmock_src_url  = 'https://googlemock.googlecode.com/files/gmock-1.7.0.zip'

MinusJN ='-j%i' % min(multiprocessing.cpu_count(), 8)
IsWindows = sys.platform == 'win32'
# All thrid party libs that can be build with CMAKE are unpackaged and built
# within a 'build' directory inside their respective folder

def which(program):
    """
    Returns the full path of to a program if available in the system PATH, None otherwise
    """
    import os
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)
    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
    return None

class Builder:
    """
    Holds all the methods needed to build code
    """
    def _detectVSVersion(self):
        """
        Detects the first available version of Visual Studio
        """
        vc_releases = [('14', '2015'), ('12', '2013'), ('11', '2012'), ('10', '2010')]
        for (vc_version, vc_release) in vc_releases:
            vcvarsbat = "C:\\Program Files (x86)\\Microsoft Visual Studio %s.0\\VC\\vcvarsall.bat" % vc_version
            if os.path.exists(vcvarsbat):
                self._vc_version = vc_version
                self._vcvarsbat = vcvarsbat
                self._vc_cmake_gen = 'Visual Studio ' + vc_version + ' ' + vc_release
                if "64" in self._arch_name:
                    self._vc_cmake_gen += ' Win64'
                break

    def _runCmd(self, cmd_args):
        """
        Runs a shell command
        """
        env = os.environ.copy()
        cmd_all = []
        if IsWindows:
            # Load visual studio environmental variables first
            if not hasattr(self, '_vcvarsbat'):
                self._detectVSVersion()
            cmd_all = [self._vcvarsbat, self._arch_name, '&&', 'set', 'CL=/MP', '&&']
        else:
            env['CXXFLAGS'] = '-fPIC'
            env['LD_LIBRARY_PATH'] = self._install_dir
        cmd_all = cmd_all + cmd_args
        print ' '.join(cmd_args)
        process = subprocess.Popen(cmd_all, env=env)
        process.wait()
        if process.returncode != 0:
            print 'Command "' + ' '.join(cmd_args) \
                + '" exitited with code ' + str(process.returncode)
            os.chdir(self._root_dir)
            sys.exit(process.returncode)

    def _runCmake(self, cmake_generator, cmakelists_path='.', extra_definitions=[]):
        """
        Runs CMake with the specified generator in the specified path with possibly some extra definitions
        """
        cmake_args = ['cmake',  \
        '-DCMAKE_INSTALL_PREFIX=' + self._install_dir,  \
        '-DCMAKE_PREFIX_PATH=' + self._install_dir,  \
        '-DCMAKE_BUILD_TYPE=' + self._build_config,  \
        '-G', cmake_generator]

        for elmt in extra_definitions:
            cmake_args.append(elmt)

        cmake_args.append(cmakelists_path)
        self._runCmd(cmake_args)

    def _setStartupProjectInVSSolution(self, project_name):
        solution_file = glob.glob(self._build_dir + '/*.sln')[0]
        sln_lines = []
        with open(solution_file) as f:
            sln_lines = f.read().splitlines()
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
        prj_lines = sln_lines[:2] + sln_lines[lin_prj_beg:lin_prj_end+1] \
            + sln_lines[2:lin_prj_beg] + sln_lines[lin_prj_end+1:]
        with open(solution_file, "w") as f:
            f.writelines(["%s\n" % item  for item in prj_lines])
        self._runCmd(['call', 'devenv', solution_file])

    def _donwloadAndExtractGMock(self, override=False):
        """
        Extract and build GMock libraries
        """
        # Download GMOCK sources if not done yet
        gmock_src_pkg = self._downloadThirdPartyLib(gmock_src_url)
        # Get the file prefix for POCO
        gmock_extract_dir = self._getThridPartyLibDirectory('gmock')

        if gmock_extract_dir is None:
            # Extract the source files
            self._extractThirdPartyLibrary(gmock_src_pkg)

    def _getThridPartyLibDirectory(self, prefix):
        """
        Get the directory where a third party library with the specified prefix name was extracted, if any
        """
        third_party_dirs = next(os.walk(self._third_party_dir))[1]
        for lib_dir in third_party_dirs:
            if prefix in lib_dir:
                return os.path.join(self._third_party_dir, lib_dir)
        return None

    def _getFileNameFromUrl(self, url):
        """
        Extracts the file name from a given URL
        """
        lib_filename = url.split('/')[-1].split('#')[0].split('?')[0]
        lib_filepath = os.path.join(self._third_party_dir, lib_filename)
        return lib_filepath

    def _downloadThirdPartyLib(self, url, ):
        """
        Download a third party dependency from the internet if is not available offline
        """
        lib_filepath = self._getFileNameFromUrl(url)
        if not os.path.exists(lib_filepath):
            print 'Downloading ' + url + ' to "' + lib_filepath + '" please wait ...'
            import urllib2
            lib_file = urllib2.urlopen(url)
            with open(lib_filepath,'wb') as output:
                 output.write(lib_file.read())
        return lib_filepath

    def _extractThirdPartyLibrary(self, lib_src_pkg):
        """
        Extracts a third party lib package source file into a directory
        """
        print 'Extracting third party library "' + lib_src_pkg + '" please wait ...'
        if 'zip' in lib_src_pkg:
            zip = zipfile.ZipFile(lib_src_pkg)
            for item in zip.namelist():
                zip.extract(item, self._third_party_dir)
            zip.close()
        else: # Assume tar archive (tgz, tar.bz2, tar.gz)
            tar = tarfile.open(lib_src_pkg, 'r')
            for item in tar:
                tar.extract(item, self._third_party_dir)
            tar.close()

    def _buildCMakeLibrary(self, cmakelists_path, extra_definitions=[], targets=[], clean_build=False):
        build_dir = os.path.join(cmakelists_path, 'build')
        # Clean and create the build directory
        if clean_build and os.path.exists(build_dir): # Remove the build directory
            shutil.rmtree(build_dir)
        if not os.path.exists(build_dir): # Create the build directory
            os.mkdir(build_dir)

        # Define CMake generator and make command
        cmake_generator = ''
        make_cmd = ''
        if IsWindows:
            cmake_generator = 'NMake Makefiles'
            make_cmd = ['set','MAKEFLAGS=', '&&', 'nmake', 'VEBOSITY=1']
        else:
            cmake_generator = 'Unix Makefiles'
            make_cmd = ['make', MinusJN, 'install']
        os.chdir(build_dir)
        cmake_cmd = ['cmake',  \
            '-DCMAKE_BUILD_TYPE=' + self._build_config,  \
            '-G', cmake_generator] + extra_definitions
        cmake_cmd.append(cmakelists_path)
        # Run CMake and Make
        self._runCmd(cmake_cmd)
        self._runCmd(make_cmd)
        for target in targets:
            self._runCmd(make_cmd + [target])
        os.chdir(self._root_dir)

    def _parseArguments(self):
        default_arch_name = 'x64'
        default_build_cfg = 'release'
        if IsWindows:
            default_arch_name = 'x86'
            default_build_cfg = 'debug'
        parser = argparse.ArgumentParser(description='Builds my cool passport photo print generator application.')
        parser.add_argument('--arch_name', help='Target platform [x86 | x64]', default=default_arch_name)
        parser.add_argument('--build_config', help='Builds the code base in [debug | release] mode', default=default_build_cfg)
        parser.add_argument('--clean', help='Cleans the whole build directory', action="store_false")
        parser.add_argument('--run_tests', help='Run existing unit tests', action="store_true")
        parser.add_argument('--run_install', help='Runs install commands', action="store_true")
        parser.add_argument('--gen_vs_sln', help='Generates Visual Studio solution and projects', action="store_true")

        args = parser.parse_args()

        self._arch_name = args.arch_name
        self._build_clean = args.clean
        self._build_config = args.build_config
        self._gen_vs_sln = args.gen_vs_sln
        self._run_tests = not args.run_tests
        self._run_install = not args.run_install

        # directory suffix for the build and release
        self._root_dir = os.path.dirname(os.path.realpath(__file__))
        self._build_dir = os.path.join(self._root_dir, 'build_' + self._build_config + '_' + self._arch_name)
        self._install_dir = os.path.join(self._root_dir, 'install_' + self._build_config + '_' + self._arch_name)
        self._third_party_dir = os.path.join(self._root_dir, 'thirdparty')
        self._third_party_install_dir = os.path.join(self._third_party_dir, 'install_' + self._build_config + '_' + self._arch_name)
        if self._gen_vs_sln:
            self._build_dir = os.path.join(self._root_dir, 'visualstudio')


    def _buildProject(self):
        # Build actions
        if self._build_clean and os.path.exists(self._build_dir):
             # Remove the build directory - clean
            shutil.rmtree(self._build_dir)
        if not os.path.exists(self._build_dir):
            # Create the build directory if doesn't exist
            os.mkdir(self._build_dir)

        # Configure build system
        make_cmd = ['make', MinusJN]
        cmake_generator = 'Unix Makefiles'
        if IsWindows:
            cmake_generator = 'NMake Makefiles'
            make_cmd = ['nmake']

        # Change directory to build directory
        os.chdir(self._build_dir)
        if self._gen_vs_sln:
            # Generating visual studio solution
            cmake_generator = self._vc_cmake_gen
            self._runCmake(cmake_generator, '..')
            self._setStartupProjectInVSSolution('ppp_test')
        else:
            # Building the project code from the command line
            self._runCmake(cmake_generator, '..')
            self._runCmd(make_cmd)
            if self._run_tests:
                self._runCmd(make_cmd + ['test'])
            if self._run_install:
                self._runCmd(make_cmd + ['install'])
            os.chdir(self._root_dir)

            if not IsWindows:
                # Build the node addon with node-gyp
                self._buildInstallAddonNodeGyp()
            # Run addon integration test
            os.chdir(self._install_dir)
            self._runCmd(['node', "./test.js"])
        os.chdir(self._root_dir)

    def __init__(self):
        # Detect OS version
        self._parseArguments()
        if IsWindows:
            self._detectVSVersion()

        # Create install directory if it doesn't exist
        if not os.path.exists(self._install_dir):
            os.mkdir(self._install_dir)

        # Build Third party libs
        self._donwloadAndExtractGMock()

        # Build this project
        self._buildProject()

b = Builder()
