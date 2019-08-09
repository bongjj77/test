# test
boost base network module server/client 
- tcp 
- udp 
- c++14
- asio(boost 1.70)
- network
- visual studio 2019
- linux/windows 

OS 

Windows 
- boost 1.70 install
    
- open ssl
    http://slproweb.com/products/Win32OpenSSL.html 1.1.1 install

- VisualStudio 2019

Linux(Ubuntu 18.04LTS)
- build/openssl/boost install
  $ apt install -y build-essential nasm autoconf libtool zlib1g-dev libssl-dev pkg-config cmake curl
  $ wget https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.gz
  $ tar -zxvf boost_1_70_0.tar.gz
  $ cd boost_1_70_0/
  $ ./bootstrap.sh
  $ ./b2
  $ dpkg -s libboost-dev | grep Version

  
- make 