# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

"""Checks the formatting of the codebase and fails if it is not correct."""

import os
import subprocess
import sys
import sysconfig

script_dir = os.path.dirname(os.path.realpath(__file__))
root_dir = os.path.join(script_dir, "..")
os.chdir(root_dir)

user_dir = sysconfig.get_path("scripts",f"{os.name}_user")
if os.name == 'nt':
    clang_format = "{}\clang-format.exe".format(user_dir)
    cmake_format = "{}\cmake-format.exe".format(user_dir)
else:
    clang_format = "{}/clang-format".format(user_dir)
    cmake_format = "{}/cmake-format".format(user_dir)

if not os.path.exists(clang_format):
    print("clang-format not found at: {}".format(clang_format))
    sys.exit(1)

if not os.path.exists(cmake_format):
    print("cmake-format not found at: {}".format(cmake_format))
    sys.exit(1)

cmake_filename='CMakeLists.txt'

# Find all the interesting files in the repository
clang_files = []
cmake_files = []

all_files = subprocess.check_output('git ls-files', shell=True, text=True).strip('\n').split('\n')

for file in all_files:
    if file.endswith('.cpp') or file.endswith('.h'):
        clang_files.append(file)
    if file.endswith(cmake_filename):
        cmake_files.append(file)

# Run clang and cmake-format on all the interesting files
unformatted_files = []
for file in clang_files:
    # When the file is unformatted, clang-format returns a zero exit code, but a non-empty stderr
    result = subprocess.check_output("{} {} -n".format(clang_format, file), shell=True, text=True, stderr=subprocess.STDOUT)
    if len(result) > 0:
        unformatted_files.append(file)
for file in cmake_files:
    # When the file is unformatted, cmake-format returns a non-zero exit code
    result = subprocess.run("{} {} --check".format(cmake_format, file), stderr=subprocess.DEVNULL, shell=True)
    if result.returncode != 0:
        unformatted_files.append(file)

unformatted_files.sort()

if unformatted_files:
    print("The following files have incorrect formatting:\n")
    for file in unformatted_files:
        print(file)
    sys.exit(1)

print("All files have correct formatting.")
