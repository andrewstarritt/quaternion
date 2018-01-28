# From https://docs.python.org/3.5/extending/newtypes.html
#
# This file is part of the Python quaternion module. It privides the setup.
#
# Copyright (c) 2018  Andrew C. Starritt
#
# The quaternion module is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# The quaternion module is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with the quaternion module.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact details:
# starritt@netspace.net.au
# PO Box 3118, Prahran East, Victoria 3181, Australia.
#

from distutils.core import setup, Extension

m = Extension("quaternion",
              sources = ["qtype/quaternion_basic.c",
                         "qtype/quaternion_object.c",
                         "qtype/quaternion_math.c",
                         "qtype/quaternion_module.c"])

setup(name="quaternion",
      version="1.0.3",
      author="Andrew Starritt",
      author_email="starritt@netspace.net.au",
      license="GPL3",
      description=""" Provides a Quaternion and associated math functions """,
      ext_modules=[m])

# end
