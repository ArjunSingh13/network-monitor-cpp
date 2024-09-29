from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake

class MyBoostProjectConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    requires = "boost/1.78.0"
    generators = "CMakeToolchain", "CMakeDeps"

    def configure(self):
        self.options["boost"].shared = False

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")
        self.copy("*.so*", dst="bin", src="lib")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
