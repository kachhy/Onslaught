<div align="center">
<img src="./img/icon_tiny.png" height:auto;"/>

[![GitHub stars](https://img.shields.io/github/stars/kachhy/Axiom?style=for-the-badge)](https://github.com/kachhy/Axiom/stargazers) [![GitHub forks](https://img.shields.io/github/forks/kachhy/Axiom?style=for-the-badge)](https://github.com/kachhy/Axiom/network) [![GitHub issues](https://img.shields.io/github/issues/kachhy/Axiom?style=for-the-badge)](https://github.com/kachhy/Axiom/issues)

[![GitHub license](https://img.shields.io/github/license/kachhy/Axiom?style=for-the-badge)](LICENSE)

[![Top Language](https://img.shields.io/github/languages/top/kachhy/Axiom?style=for-the-badge&color=blue)](https://github.com/kachhy/Axiom)

# Axiom
**A UCI-compliant chess engine built in C++.**
</div>

## 📖 Overview

Axiom is a Universal Chess Interface (UCI) compliant chess engine developed in C++. Designed for performance, Axiom can be used with various UCI-compatible chess graphical user interfaces (GUIs) to play, analyze, and find optimal moves in standard chess positions.

## 🚀 Quick Start

Follow these steps to build and run the Axiom chess engine.

### Prerequisites

-   **C++ Compiler**: A C++17 compatible compiler (e.g., GCC 7+ or Clang 5+).
-   **Make**: GNU Make utility.

### Installation

1.  **Clone the repository**
    ```bash
    git clone https://github.com/kachhy/Axiom.git
    cd Axiom
    ```

2.  **Build the engine**
    The `Makefile` will compile the source code and create the `axiom` executable.
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

-   The [Chess Programming Wiki](https://www.chessprogramming.org) for being an invaluable resource for chess engine development.
-   [OpenBench](https://github.com/AndyGrant/OpenBench) is a great SPRT testing framework that we heavily rely on to test changes.
-   The [Bullet](github.com/jw1912/bullet) NNUE trainer for training our networks and being incredibly intuitive and easy to use.
-   [Incbin](https://github.com/graphitemaster/incbin) which we use to include our networks in our binaries.
