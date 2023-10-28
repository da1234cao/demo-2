
## 环境准备

openssl安装。

```shel
# 安装openssl库
vcpkg install openssl:x64-windows

Computing installation plan...
The following packages are already installed:
    openssl:x64-windows -> 3.1.1
openssl:x64-windows is already installed
Total install time: 242 us
The package openssl is compatible with built-in CMake targets:

    find_package(OpenSSL REQUIRED)
    target_link_libraries(main PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# 将openssl命令所在目录,临时添加到搜索路径中
## 我系统中有很多openssl.exe。
## 其中之一的openssl.exe位于D:\tools\vcpkg\downloads\tools\perl\5.32.1.1\c\bin目录
## powershell中修改path
$Env:Path += ';D:\tools\vcpkg\downloads\tools\perl\5.32.1.1\c\bin'

# openssl命令：https://www.openssl.org/docs/man1.0.2/man1/openssl.html
openssl version -a
OpenSSL 1.1.1i  8 Dec 2020
built on: Sun Jan 24 10:44:14 2021 UTC
platform: mingw
options:  bn(64,32) rc4(8x,mmx) des(long) idea(int) blowfish(ptr)
compiler: gcc -m32 -Wall -O3 -fomit-frame-pointer -DL_ENDIAN -DOPENSSL_PIC -DOPENSSL_CPUID_OBJ -DOPENSSL_BN_ASM_PART_WORDS -DOPENSSL_IA32_SSE2 -DOPENSSL_BN_ASM_MONT -DOPENSSL_BN_ASM_GF2m -DSHA1_ASM -DSHA256_ASM -DSHA512_ASM -DRC4_ASM -DMD5_ASM -DRMD160_ASM -DAESNI_ASM -DVPAES_ASM -DWHIRLPOOL_ASM -DGHASH_ASM -DECP_NISTZ256_ASM -DPOLY1305_ASM -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -D_MT -DZLIB -DOPENSSL_USE_APPLINK -DNDEBUG -I/z/extlib/_openssl111_/include -DOPENSSLBIN="\"/z/extlib/_openssl111_/bin\""
OPENSSLDIR: "Z:/extlib/_openssl111_/ssl"
ENGINESDIR: "Z:/extlib/_openssl111_/lib/engines-1_1"
Seeding source: os-specific
```