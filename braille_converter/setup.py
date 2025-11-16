"""
Setup script for the Braille Converter library.
"""

from setuptools import setup, find_packages
from pathlib import Path

# Read the README file
readme_file = Path(__file__).parent / "README.md"
long_description = ""
if readme_file.exists():
    with open(readme_file, "r", encoding="utf-8") as f:
        long_description = f.read()

setup(
    name="braille-converter",
    version="1.0.0",
    author="Senior Design Group 6",
    author_email="",
    description="A Python library for converting text to 6-dot braille representation",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/yourusername/braille-converter",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: Education",
        "Topic :: Text Processing",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
    ],
    python_requires=">=3.8",
    install_requires=[],
    extras_require={
        "dev": [
            "pytest>=7.0.0",
            "pytest-cov>=4.0.0",
            "black>=23.0.0",
            "flake8>=6.0.0",
            "mypy>=1.0.0",
        ],
    },
    entry_points={
        "console_scripts": [
            "text-to-braille=braille_converter.cli:main",
        ],
    },
    project_urls={
        "Bug Reports": "https://github.com/yourusername/braille-converter/issues",
        "Source": "https://github.com/yourusername/braille-converter",
    },
)

