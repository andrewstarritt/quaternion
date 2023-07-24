# From https://docs.python.org/3.5/extending/newtypes.html
#
# This file is part of the Python quaternion module. It privides the setup.
#
# Note: to upload to aspypi, run command:
#
#    python setup.py sdist upload -r aspypi
#
# Copyright (c) 2018-2022  Andrew C. Starritt
#
# The quaternion module is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# The quaternion module is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with the quaternion module.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact details:
# andrew.starritt@gmail.com
# PO Box 3118, Prahran East, Victoria 3181, Australia.
#

import sys
from distutils.core import setup, Extension
import re

with open("qtype/quaternion_module.c", 'r') as f:
    version = re.search(r'__version__ "(.*)"', f.read()).group(1)

m = Extension("quaternion",
              sources = ["qtype/quaternion_basic.c",
                         "qtype/quaternion_object.c",
                         "qtype/quaternion_array.c",
                         "qtype/quaternion_array_iter.c",
                         "qtype/quaternion_math.c",
                         "qtype/quaternion_utilities.c",
                         "qtype/quaternion_module.c"])

setup(name="quaternion",
      version=version,
      author="Andrew Starritt",
      author_email="andrew.starritt@gmail.com",
      license="LGPL3",
      description="""Provides a Quaternion type and associated maths functions together
                     with a QuaternionArray type.
                  """,
      ext_modules=[m])

# end
