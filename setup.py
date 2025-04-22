from setuptools import setup, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "module_ai",
        ["module_ai.cpp"],
        cxx_std=17,
    ),
]

setup(
    name="module_ai",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
)