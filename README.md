# Project

Skill

- c++14 / asio(boost 1.85) / network / visual studio 2022(v143)/ linux / windows

- Rtmp / HLS / Mpeg-Dash / Cmaf / http(s) / fMP4 / TS / Ultra Low Latency

Program 

  SimpleMediaStreamingServer : simple live media streaming server

     Input : rtmp(obs,xsplit...)

	 Output : hls/mpeg-dash/cmaf(ull)

	 Play : Chrome Browser, Media Player Applications

  EchoServer/EchoTcpCliet : boost base network server/client test 


Source
    $  git clone https://github.com/bongjj77/test.git


Windows build

		- boost 1.85.0 install
		   visual studio 64bit cmd windows
		   $ boostrap.bat
		   $ b2 
		   // toolset=msvc-14.1 address-model=64 link=static threading=multi runtime-link=shared 
    
		- open ssl

			http://slproweb.com/products/Win32OpenSSL.html 3.3.0 install
    
		- VisualStudio 2022 directory setting and solution build


Linux(Ubuntu 18.04LTS) build

		- build/openssl/boost install

			$ apt install -y build-essential nasm autoconf libtool zlib1g-dev libssl-dev pkg-config cmake curl
			$ wget https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_85_0.tar.gz
			$ tar -zxvf boost_1_85_0.tar.gz
			$ cd boost_1_85_0/
			$ ./bootstrap.sh
			$ ./b2
			$ dpkg -s libboost-dev | grep Version

		- project build 

			$ cd /SimpleMediaStreamingServer/linux_build
			$ make && make install

			$ cd /EchoServer/linux_build 
			$ make && make install

			$ cd /EchoTcpClinent/linux_build 
			$ make && make install
	    
Execute 

