# Calliope

クロスプラットフォーム対応の MIDI シーケンサー

[![License](https://img.shields.io/badge/license-AGPL--3.0-blue.svg)](LICENSE)
![Status](https://img.shields.io/badge/status-WIP-yellow)

## 概要

Calliope は、[JUCE](https://juce.com/) ベースの MIDI シーケンサーです。VST3 プラグインのホスティングに対応しています。

## システム要件

- [CMake](https://cmake.org/) 3.22 以上
- [Ninja](https://ninja-build.org/)
- C++20 対応コンパイラ

## ビルド

```bash
git clone --recursive https://github.com/oruponu/calliope.git
cd calliope
cmake --preset default
cmake --build build
```

## ライセンス

[AGPL-3.0 License](LICENSE)
