from setuptools import find_packages, setup

setup(
    name="mem-hint",
    version="0.1.0",
    author="Manish KL",
    author_email="mlachwani@gmail.com",
    description="Python client for /dev/mem_hint AI memory hint interface",
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    url="https://github.com/manishklach/mem-hint",
    packages=find_packages(),
    python_requires=">=3.8",
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "Topic :: System :: Hardware",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Programming Language :: Python :: 3",
        "Operating System :: POSIX :: Linux",
    ],
    keywords=["memory", "llm", "inference", "linux", "kernel",
              "mem-hint", "dram", "ai-infrastructure"],
)
