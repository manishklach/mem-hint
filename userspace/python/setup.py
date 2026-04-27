from setuptools import setup, find_packages

with open("README.md") as f:
    long_desc = f.read()

setup(
    name="mem-hint",
    version="0.1.0",
    author="Manish KL",
    description="Python client for /dev/mem_hint AI memory hint interface",
    long_description=long_desc,
    long_description_content_type="text/markdown",
    url="https://github.com/manishklach/mem-hint",
    packages=find_packages(),
    python_requires=">=3.8",
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Topic :: System :: Hardware",
        "Programming Language :: Python :: 3",
        "Operating System :: POSIX :: Linux",
    ],
    keywords=["mem-hint", "llm", "inference", "linux",
              "kernel", "dram", "ai-infrastructure"],
)
