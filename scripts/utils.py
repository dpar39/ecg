import os
import sys
import json
import shutil
from collections import OrderedDict


def read_file(file_path, mode='r'):
    if not os.path.isfile(file_path):
        return None
    with open(file_path, mode) as fp:
        return fp.read()


def write_file(file_path, content, mode='w'):
    folder = os.path.dirname(file_path)
    if folder and not os.path.isdir(folder):
        os.makedirs(folder)
    with open(file_path, mode) as fp:
        fp.write(content)


def read_json(file_path):
    if not os.path.isfile(file_path):
        return None
    with open(file_path, encoding='utf-8') as fp:
        return json.load(fp, object_hook=OrderedDict)


def write_json(file_path, data):
    with open(file_path, 'w') as fp:
        json.dump(data, fp)


def which(program):
    """
    Returns the full path of to a program if available in the system PATH, None otherwise
    """
    def is_exe(fpath):
        """
        Returns true if the file can be executed, false otherwise
        """
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)
    fpath, _ = os.path.split(program)
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


def platform_name():
    platform = sys.platform
    if platform == "linux" or platform == "linux2":
        return 'linux'
    elif platform == "darwin":
        return 'osx'
    elif platform == "win32":
        return 'windows'
    else:
        return None


def check_platform(p_name):
    return platform_name() == p_name


def is_windows():
    return check_platform('windows')


def is_posix():
    return check_platform('osx') or check_platform('linux')


def link_file(src_file_path, dst_link):
    if not os.path.exists(src_file_path):
        raise FileNotFoundError(src_file_path)
    print('Creating link for file "%s" in "%s"' % (src_file_path, dst_link))
    if check_platform('windows'):
        shutil.copyfile(src_file_path, dst_link)
        link_cmd = 'mklink "%s" "%s"' % (dst_link, src_file_path)
    else:
        link_cmd = 'ln -sf "%s" "%s"' % (src_file_path, dst_link)
    os.system(link_cmd)


def make_path(*args):
    return os.path.join(*args).replace('\\', '/')
