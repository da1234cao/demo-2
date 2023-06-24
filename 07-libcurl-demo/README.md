[toc]

# 前言

[libcurl库](https://curl.se/libcurl/)是一个免费且易于使用的客户端 URL 传输库。它支持多种协议，高度跨平台。文档也不错。

本文记录下libcurl的安装与libcurl API的入门使用。

# libcurl的安装

windows和linux上libcurl的安装。

## windows上安装libcurl

### 不成功的尝试

我不喜欢在windows上编程，因为安装第三方库比较麻烦。这里先尝试了能否简单的安装。尝试使用windows的包管理器[winget](https://learn.microsoft.com/zh-cn/windows/package-manager/winget/)安装。

```shell
# 先查看先是否存在curl的包
winget.exe search curl
名称         ID           版本    源
------------------------------------------
Curling Live 9NBLGGH6CX9F Unknown msstore
SimpleCurl   9NBLGGH5G9QZ Unknown msstore
CollaChat    9P59NJL6KPWS Unknown msstore
cURL         cURL.cURL    8.0.1   winget

# 安装curl
winget.exe install cURL.cURL

# winget无法查看已安装的包中有哪些内容，又安装到什么位置
# winget作为包管理器，还不是很好用
```

winget安装curl的过程中，可能会失败。看winget的输出，它是先将包下载到[Temp](https://answers.microsoft.com/en-us/windows/forum/all/where-is-the-temporary-folder/44a039a5-45ba-48dd-84db-fd700e54fd56)路径，然后复制到指定位置。我之前改过环境变量`TEMP`和`TEM`,将tmp目录放在D盘。这会导致失败。需要将tmp目录改回默认路径。我不知道为什么，也没找到参考链接，但确实有效。。

上面安装过后，可以在`powershell`中使用curl.exe。但是，我用evertything搜了下，winget没有将curl的头文件和库拷贝到系统路径中去。**所以，对于想要在C/C++中使用libcurl的人说，winget无法安装libcurl，它只能安装curl.exe**。

---

### vcpkg的安装

参考[install curl in window](https://everything.curl.dev/get/windows)，windows中安装libcurl有`MSYS2`和`vcpkg`两种方法。我不是很喜欢`MSYS2`,而且对于想要在CMake中使用，参考[FindCURL](https://cmake.org/cmake/help/latest/module/FindCURL.html)对应的[FindCURL.cmake](https://github.com/Kitware/CMake/blob/master/Modules/FindCURL.cmake)，`vcpkg`无疑是更好的选择。

所以，这里我们先安装下[vcpkg](https://github.com/microsoft/vcpkg/blob/master/README_zh_CN.md)。vcpkg是个跨平台的C/C++库管理器。参见它的[Release](https://github.com/microsoft/vcpkg/releases),可以看到支持很多库。很好！！。~~但是为啥不把给vcpkg自己做一个安装包。还得从源码编译。执行了脚本，倒也没有源码编译~~

```shell
cd D:\tools\
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
# 然后将vcpkg.exe放到系统的查找路径中
```

---

### 安装libcurl

```shell
D:\tools\vcpkg> .\vcpkg.exe install curl:x64-windows
...
-- Fixing pkgconfig file: D:/tools/vcpkg/packages/curl_x64-windows/debug/lib/pkgconfig/libcurl.pc
-- Installing: D:/tools/vcpkg/packages/curl_x64-windows/share/curl/vcpkg-cmake-wrapper.cmake
-- Installing: D:/tools/vcpkg/packages/curl_x64-windows/share/curl/copyright
-- Performing post-build validation
...

# 可以看到libcurl被安装到D:\tools\vcpkg\packages目录;阅读下libcurl.pc，可以看到更确切的信息

D:\tools\vcpkg> .\vcpkg integrate install
Applied user-wide integration for this vcpkg root.
CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"

All MSBuild C++ projects can now #include any installed libraries. Linking will be handled automatically. Installing new libraries will make them instantly available.
```

按照提示，后面时候cmake的时候，添加上`-DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake`即可。[CMAKE_TOOLCHAIN_FILE](https://cmake.org/cmake/help/latest/variable/CMAKE_TOOLCHAIN_FILE.html), 是在 CMake 运行早期读取的文件的路径，该路径指定编译器和工具链实用程序的位置，以及其他目标平台和编译器相关信息。如果在首次build tree时设置了这个环境变量，它将由CMAKE_TOOLCHAIN_FILE环境变量的值进行初始化。所以，为了以后更加方便的时候vcpkg安装的库，而不用记住要使用-D参数添加这个路径，**最好的方法是在环境变量中创建一个`CMAKE_TOOLCHAIN_FILE`**。

如果想要在特定的编译器中，如vscode,vs中设置CMAKE_TOOLCHAIN_FILE，以及如何在cmake中使用，可参考这几个类似的链接：[Using vcpkg with CMake](https://github.com/microsoft/vcpkg/blob/master/README.md#using-vcpkg-with-cmake)、[Linking libcurl with CMake on Windows](https://stackoverflow.com/questions/60971350/linking-libcurl-with-cmake-on-windows)、[find_package: Could NOT find CURL](https://github.com/microsoft/vcpkg/issues/27812)

---

## Linux上安装libcurl

多好，一行命令搞定。

```shell
sudo apt install libcurl4-openssl-dev
```

---

# curl的简单使用

## 相关参考：

* [curl入门教程: How to Get started with libcURL](https://www.bilibili.com/video/BV1Qa4y1w7vc/) -- 挺好的入门视频，**本节的介绍顺序，来自这个视频**。
* [The libcurl API](https://curl.se/libcurl/c/) -- 本节的示例代码，来自该链接中的代码。
* [HTTP with libcurl](https://everything.curl.dev/libcurl-http) -- curl支持多种协议,目前仅尝试http协议

---

## Libcurl API概述

在开始使用libcurl api之前，我们需要相对其有个整体上的了解，见：[libcurl API overview](https://curl.se/libcurl/c/libcurl.html)

libcurl提供了不同组的API。[libcurl-easy](https://curl.se/libcurl/c/libcurl-easy.html):当需要进行简单同步的传输,可以使用这组API。[libcurl-multi](https://curl.se/libcurl/c/libcurl-multi.html):可以在一个线程内启动多个传输,且不阻塞。[libcurl-share](https://curl.se/libcurl/c/libcurl-share.html):可以在多个curl句柄中共享数据，如cooki,dns缓存,TLS session,但是需要互斥调用。[libcurl-url](https://curl.se/libcurl/c/libcurl-url.html):提供用于解析和生成 URL 的函数。[libcurl-ws](https://curl.se/libcurl/c/libcurl-ws.html):接口提供了接收和发送 WebSocket 数据的函数。

libcurl API的简单使用流程是：[curl_easy_init](https://curl.se/libcurl/c/curl_easy_init.html)创建一个句柄; [curl_easy_setopt](https://curl.se/libcurl/c/curl_easy_setopt.html)给创建的句柄设置选项; 然后使用[curl_easy_perform](https://curl.se/libcurl/c/curl_easy_perform.html)进行阻塞式传输。

更多的信息，参见上面的参考链接。知晓一个大概就好，下面进行demo尝试。

---

## 最小的curl_easy示例

参考：[Libcurl api overview](https://curl.se/libcurl/c/libcurl.html)、[Simple HTTPS GET](https://curl.se/libcurl/c/https.html)

```cpp
/* frome:
 * https://raw.githubusercontent.com/curl/curl/master/docs/examples/https.c
 * Simple HTTPS GET
 */
#include <curl/curl.h>
#include <stdio.h>

int main(void) {
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://example.com/");

#ifdef SKIP_PEER_VERIFICATION
    /*
     * If you want to connect to a site who is not using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    /*
     * If the site you are connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

    /* cache the CA cert bundle in memory for a week */
    curl_easy_setopt(curl, CURLOPT_CA_CACHE_TIMEOUT, 604800L);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();

  return 0;
}
```

虽然比较无聊，但还是简单介绍下上面代码中用到的API。

* [curl_global_init](https://curl.se/libcurl/c/curl_global_init.html): 设置 libcurl 需要的程序环境。可能是线程安全的，也可能不是，和libcurl版本有关。建议使用`CURL_GLOBAL_DEFAULT`来初始化 SSL 和 Win32 socket。如果此函数返回非零值，则表示出现了错误，无法使用其他 curl 函数。
* [curl_global_cleanup](https://curl.se/libcurl/c/curl_global_cleanup.html): 释放curl_global_init获取的资源。在使用 libcurl 完成后调用。没有返回值。
* [curl_easy_init](https://curl.se/libcurl/c/curl_easy_init.html): 它返回一个 CURL 简单句柄, 句柄被其他函数使用。果您还没有调用 curl_global_init，那么 curl_easy_init 会自动调用它。这在多线程情况下可能是致命的，因为 curl_global_ init 不是线程安全的，并且由于没有相应的清理，它可能导致资源问题。尽量自己正确地调用 curl_global_init。如果此函数返回 NULL，则表示出错，无法使用其他 curl 函数。
* [curl_easy_cleanup](https://curl.se/libcurl/c/curl_easy_cleanup.html): 关闭curl_easy_init创建句柄。这可能会关闭此句柄使用过的所有连接。句柄调用过这个函数后，这个句柄就不能再使用了。curl_easy_cleanup 会杀死句柄和与之相关的所有内存！没有返回值。
* [curl_easy_perform](https://curl.se/libcurl/c/curl_easy_perform.html): 阻塞执行传输。在 curl_easy_init 和所有 curl_easy_setopt 调用完成后调用此函数，它将按照选项中的描述执行传输。对于一个句柄，可以多次调用curl_easy_perform。但是句柄是需要互斥访问的资源，所以不要在多线程中使用curl_easy_perform操作相同的句柄。CURLE_OK(0)表示一切正常，非零表示发生错误。
* 关于`CURLOPT_SSL_VERIFYHOST`、`CURLOPT_SSL_VERIFYHOST`、`CURLOPT_CA_CACHE_TIMEOUT`，如果知道[TLS握手过程](https://da1234cao.blog.csdn.net/article/details/124621077)，很容易明白，这里不再赘述。

---

## 更多demo

这里是官方的demo示例：[libcurl - small example snippets](https://curl.se/libcurl/c/example.html)。

我看了几个。

* [http-post](https://curl.se/libcurl/c/http-post.html): 上一节是GET请求。这个链接中是POST请求的示例。
* [http2-upload](https://curl.se/libcurl/c/http2-upload.html): 上传文件。（吃完饭困了,待需要的时候再看。

入门篇，over