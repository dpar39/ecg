import argparse
import shutil
import os

# Source files generator

# Templates

iface_template_h = \
"""
#pragma once
#include <memory>

class $IFACE_NAME;
typedef std::shared_ptr<$IFACE_NAME> $IFACE_NAMESPtr;

class $IFACE_NAME
{
public:
    virtual ~$IFACE_NAME() = default;
};
"""

class_template_h = \
"""
#pragma once
#include <memory>

#include "$IFACE_NAME.h"

class $CLASS_NAME;
typedef std::shared_ptr<$CLASS_NAME> $CLASS_NAMESPtr;

class $CLASS_NAME : public $IFACE_NAME
{
public:
    ~$CLASS_NAME() { }
};
"""

class_template_cpp = \
"""
#include "$CLASS_NAME.h"

"""

mock_template_h = \
"""
#include "$CLASS_NAME.h"

#include <gmock/gmock.h>

class Mock$CLASS_NAME : public $IFACE_NAME
{

};
"""

test_template_cpp = \
"""
#include <gtest/gtest.h>

#include "$CLASS_NAME.h"

class $CLASS_NAMETests : public ::testing::Test
{
protected:
    $CLASS_NAMESPtr m_sut;

public:
    void SetUp()
    {
    }
};

TEST_F($CLASS_NAMETests, Test1)
{

}
"""

def write_file(filename, content, make_backup):
    """
    Writes the content string to the file specified in filename
    If the file already exists, a backup will be created.
    """
    if make_backup and os.path.exists(filename):
        idx = 1
        bak_file = '%s.%d.bak' % (filename, idx)
        while os.path.exists(bak_file):
            idx += 1
            bak_file = '%s.%d.bak' % (filename, idx)
        shutil.copyfile(filename, bak_file)

    with open(filename, 'w') as fstream:
        fstream.write(content)

def template(content, keyvalues):
    """
    Substitutes template values by the regex
    """
    for keyval in keyvalues:
        content = content.replace(keyval[0], keyval[1])
    return content

class Generator:
    """
    Generates C++ header and sources for a given C++ class
    """
    def parse_args(self):

        parser = argparse.ArgumentParser(description='Generates header and source C++ files.')
        parser.add_argument('-c', '--class',     dest='className', help='Name of the class to be generated')
        parser.add_argument('-p', '--prefix',    dest='prjPrefix', help='Project directory prefix. E.g. mylib')
        parser.add_argument('-ni', '--no_iface',  dest='noIface',   help='Skip generating interface header file under <prefix>/include/', action='store_true')
        parser.add_argument('-nm', '--no_mock',   dest='noMock',    help='Skip generating mock header file under <prefix>/test/', action='store_true')
        parser.add_argument('-nt', '--no_test',   dest='noTest',    help='Skip generating test fixture source file under <prefix>/test/', action='store_true', default=True)
        parser.add_argument('-nb', '--no_backup', dest='noBackup',  help='Skip making back up of files to be overwritten', action='store_true')

        self.args = parser.parse_args()

    def generate(self):
        """
        Generates interface, implementing class, mock and test fixture
        """
        prjPrefix = self.args.prjPrefix
        className = self.args.className
        make_backup = not self.args.noBackup
        ifaceName = 'I%s' % (className)
        mapp = {('$CLASS_NAME', className), ('$IFACE_NAME', ifaceName)}

        include_dir = 'include'
        src_dir = 'src'
        test_dir = 'test'

        # Make standard header and source
        header_filename = os.path.join(prjPrefix, include_dir, '%s.h' % (className))
        header_content = template(class_template_h, mapp)
        write_file(header_filename, header_content, make_backup)

        source_filename = os.path.join(prjPrefix, src_dir, '%s.cpp' % (className))
        source_content = template(class_template_cpp, mapp)
        write_file(source_filename, source_content, make_backup)

        if not self.args.noIface:
            iface_filename = os.path.join(prjPrefix, include_dir, 'I%s.h' % (className))
            iface_content = template(iface_template_h, mapp)
            write_file(iface_filename, iface_content, make_backup)

        if not self.args.noMock:
            mock_filename = os.path.join(prjPrefix, test_dir, 'Mock%s.h' % (className))
            mock_content = template(mock_template_h, mapp)
            write_file(mock_filename, mock_content, make_backup)

        if not self.args.noTest:
            test_filename = os.path.join(prjPrefix, test_dir, '%sTests.h' % (className))
            test_content = template(test_template_cpp, mapp)
            write_file(test_filename, test_content, make_backup)

    def __init__(self):
        self.parse_args()
        self.generate()

generator = Generator()