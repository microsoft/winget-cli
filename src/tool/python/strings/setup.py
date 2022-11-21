import setuptools

setuptools.setup(
    name = "%",
    version = "1.0a0",
    description="Generated Python/WinRT package",
    license="MIT",
    url="http://github.com/Microsoft/xlang",
    package_data={ "%":["_%.pyd"] },
    packages = setuptools.find_namespace_packages(where='.', include=("%*")))
