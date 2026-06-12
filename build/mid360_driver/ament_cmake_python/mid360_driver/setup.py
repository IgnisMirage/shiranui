from setuptools import find_packages
from setuptools import setup

setup(
    name='mid360_driver',
    version='0.2.0',
    packages=find_packages(
        include=('mid360_driver', 'mid360_driver.*')),
)
