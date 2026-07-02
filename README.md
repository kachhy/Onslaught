<div align="center">
<img src="./img/onslaught.png" height:auto;"/>

[![GitHub stars](https://img.shields.io/github/stars/kachhy/Onslaught?style=for-the-badge)](https://github.com/kachhy/Onslaught/stargazers) [![GitHub forks](https://img.shields.io/github/forks/kachhy/Onslaught?style=for-the-badge)](https://github.com/kachhy/Onslaught/network) [![GitHub issues](https://img.shields.io/github/issues/kachhy/Onslaught?style=for-the-badge)](https://github.com/kachhy/Onslaught/issues)

[![GitHub license](https://img.shields.io/github/license/kachhy/Onslaught?style=for-the-badge)](LICENSE)

[![Top Language](https://img.shields.io/github/languages/top/kachhy/Onslaught?style=for-the-badge&color=blue)](https://github.com/kachhy/Onslaught)

# Onslaught
**A UCI-compliant chess engine built in C++.**
</div>

## 📖 Overview

Onslaught is a Universal Chess Interface (UCI) compliant chess engine developed in C++. Designed for performance, Onslaught can be used with various UCI-compatible chess graphical user interfaces (GUIs) to play, analyze, and find optimal moves in standard chess positions.

## 🚀 Quick Start

Follow these steps to build and run the Onslaught chess engine.

### Prerequisites

-   **C++ Compiler**: A C++17 compatible compiler (e.g., GCC 7+ or Clang 5+).
-   **Make**: GNU Make utility.

### Installation

1.  **Clone the repository**
    ```bash
    git clone https://github.com/kachhy/Onslaught.git
    cd Onslaught
    ```

2.  **Build the engine**
    The `Makefile` will compile the source code and create the `Onslaught` executable.
    ```bash
    make release
    ```
    To compile with debug symbols for easier debugging:
    ```bash
    make debug
    ```
    Or if you want to build for your machine specifically
    ```
    make native
    ```


## 📄 License & Contributing

This project is licensed under the [GNU General Public License v3.0](LICENSE) - see the [LICENSE](LICENSE) file for details.

There are two main ways to contribute to the project.

### Contributing Code
To contribute code
1.  Fork the repository
2.  Make your changes
3.  Submit tests on the [OpenBench instance](https://openbench.kaichung.dev). All functional changes must pass an LTC test and all non-functional changes or optimizations must pass a non-regression LTC test.
4.  Make a pull request with the test information (links to the tests, or the info blocks)
5.  Get feedback!

### Contributing Compute
Running all of these SPRT tests takes a lot of compute. To contribute compute to our OpenBench instance, please refer to the [OpenBench wiki](https://github.com/AndyGrant/OpenBench/wiki). Ensure you have an account, use those credentials, and use [https://openbench.kaichung.dev](https://openbench.kaichung.dev) as the endpoint.

## 🙏 Acknowledgments
- [Chess Programming Wiki](https://www.chessprogramming.org) - an invaluable reference that guided much of our development.
- [Ethereal](https://github.com/AndyGrant/Ethereal) - a great source of inspiration, both in strength and in code clarity.
- [Bullet](https://github.com/jw1912/bullet) - the NNUE trainer we used to train our networks; intuitive and easy to work with.
- [OpenBench](https://github.com/AndyGrant/OpenBench) - the SPRT testing framework we rely on for all our testing.
- [Incbin](https://github.com/graphitemaster/incbin) - used to embed our networks directly into our binaries.
- [CodeMonkeyKing's YouTube series](https://www.youtube.com/watch?v=QUNP-UjujBM&list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs) - a great starting point for learning chess engine implementation.
