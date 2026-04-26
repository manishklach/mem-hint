# Patent Pending: Indian Patent Application No. 202641053160

from pathlib import Path

from setuptools import find_packages
from setuptools import setup


README = Path(__file__).with_name("README.md").read_text(encoding="utf-8")


setup(
    name="mem-hint",
    version="0.1.0",
    author="Manish KL",
    description="Python client for /dev/mem_hint AI memory hint interface",
    long_description=README,
    long_description_content_type="text/markdown",
    python_requires=">=3.8",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python :: 3",
    ],
)
