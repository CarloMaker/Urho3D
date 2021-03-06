#
# Copyright (c) 2008-2013 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Ensure we are in project root directory
cd $( dirname $0 )

# Create out-of-source build directory
SOURCE=`pwd`/Source
BUILD=Build
if [ $1 == "-DIOS=1" ]; then
    BUILD=ios-Build
fi
cmake -E make_directory $BUILD

# Create project with the Xcode generator
cmake -E chdir $BUILD cmake -G "Xcode" $@ $SOURCE

if [ $1 == "-DIOS=1" ]; then
    # Due to a bug in the CMake/Xcode generator where it has wrongly assumed the IOS bundle structure to be the same as MacOSX bundle structure,
    # below temporary fix is required in order to solve the auto-linking issue when dependent libraries are changed
    sed -i.bak 's/\/Contents\/MacOS//g' $BUILD/CMakeScripts/XCODE_DEPEND_HELPER.make
    echo -e "\tsed -i.bak 's/\/Contents\/MacOS//g' CMakeScripts/XCODE_DEPEND_HELPER.make" >> $BUILD/CMakeScripts/ReRunCMake.make
fi

# vi: set ts=4 sw=4 expandtab:
