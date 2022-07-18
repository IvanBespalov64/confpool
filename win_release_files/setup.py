from setuptools import setup

setup(
    name='pyxyz',
    version='1.3{ver}.1',
    author='Nikolai Krivoshchapov',
    python_requires='==3.{ver}.*',
    packages=['pyxyz'],
    package_data={'pyxyz': ['__init__.py', 'confpool.pyd', '*.dll', 'test/*']}
)
