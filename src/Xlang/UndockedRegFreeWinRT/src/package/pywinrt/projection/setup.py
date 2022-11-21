import setuptools

# Extract the build revision information from Build_BuildNumber environment variable. 
# This relies on the format of the pipeline name being of the format: <build text>.$(Date:yy).$(Date:MMdd).$(DayOfYear).$(Rev:r)

import os
buildNumber = os.environ["Build_BuildNumber"].split('.')
year = buildNumber[1]
doy = buildNumber[3]
rev = buildNumber[4]

long_description = """# Python/WinRT

The Windows Runtime Python Projection (Python/WinRT) enables Python developers to access
[Windows Runtime APIs](https://docs.microsoft.com/uwp/api/) directly from Python in a natural
and familiar way.

## Getting Started

### Prerequisites

* [Windows 10](https://www.microsoft.com/windows), October 2018 Update or later.
* [Python for Windows](https://docs.python.org/3.7/using/windows.html), version 3.7 or later
* [pip](https://pypi.org/project/pip/), version 19 or later

### Installing

Python/WinRT can be installed from the [Python Package Index](https://pypi.org/) via pip. Assuming
pip is on the path, Python/WinRT can be installed from the command line with the following command:

``` shell
> pip install winrt
```

You can test that Python/WinRT is installed correctly by launching Python and running the following
snippet of Python code. It should print "https://github.com/Microsoft/xlang/tree/master/src/tool/python"
to the console.

``` python
import winrt.windows.foundation as wf
u = wf.Uri("https://github.com/")
u2 = u.combine_uri("Microsoft/xlang/tree/master/src/tool/python")
print(str(u2))
```

For more information on accessing Windows Runtime APIs from Python, please visit
[Python/WinRT's homepage](https://github.com/Microsoft/xlang/tree/master/src/package/pywinrt/projection). 

For an end-to-end sample of using Python/WinRT, please see the
[WinML Tutorial](https://github.com/Microsoft/xlang/tree/master/samples/python/winml_tutorial)
in the samples folder of the xlang GitHub repo.
"""

setuptools.setup(
    name = "winrt",
    version = "1.0.{0}{1}.{2}".format(year, doy, rev),
    description="Access Windows Runtime APIs from Python",
    long_description=long_description,
    long_description_content_type="text/markdown",
    license="MIT",
    author='Microsoft Corporation',
    url="https://github.com/Microsoft/xlang/tree/master/src/package/pywinrt/projection",
    classifiers=[
            'Development Status :: 4 - Beta',
            'Environment :: Win32 (MS Windows)',
            'License :: OSI Approved :: MIT License',
            'Operating System :: Microsoft :: Windows :: Windows 10',
            'Programming Language :: Python :: 3.7',
            'Programming Language :: Python :: 3.8',
            'Programming Language :: Python :: 3.9',
            'Programming Language :: Python :: Implementation :: CPython',
            'Topic :: System :: Operating System',
        ],
    package_data={ "winrt":["_winrt.pyd"] },
    packages = setuptools.find_namespace_packages(where='.', include=("winrt*")))
