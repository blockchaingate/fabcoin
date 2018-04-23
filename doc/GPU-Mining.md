# Fabcoin GPU mining 

# Why Equihash?


we have started using Equihash as the proof-of-work for block mining in Fabcoin .

Equihash is a Proof-of-Work algorithm devised by Alex Biryukov and Dmitry Khovratovich. It is based on a computer science and cryptography concept called the Generalized Birthday Problem.  https://en.wikipedia.org/wiki/Equihash

# WHY ARE WE USING IT?
Equihash has very efficient verification. This could in the future be important for light clients on constrained devices, or for implementing a Fabcoin client inside Ethereum (like BTC Relay, but for Fabcoin ).

Equihash is a memory-oriented Proof-of-Work, which means how much mining you can do is mostly determined by how much RAM you have. We think it is unlikely that anyone will be able to build cost-effective custom hardware (ASICs) for mining in the foreseeable future.

We also think it is unlikely that there will be any major optimizations of Equihash which would give the miners who know the optimization an advantage. This is because the Generalized Birthday Problem has been widely studied by computer scientists and cryptographers, and Equihash is close to the Generalized Birthday Problem. That is: it looks like a successful optimization of Equihash would be likely also an optimization of the Generalized Birthday Problem.

Nevertheless, we can’t know for certain that Equihash is safe against these issues, and we may change the Proof-of-Work again, if we find some flaw in Equihash or if we find another Proof-of-Work algorithm which offers higher assurance.

# HOW CAN I MINE?
The same way as before! Just add gen=1 to your config file, or run ./src/fabcoindd -gen, this will start cpu mining process. And for better result, should choose GPU mining. 

   
## GPU mining hardware requirement  

We developed GPU mining in OpenCL and Ubuntu 16.04 system, and has been tested on AMD Rx480 and Nvidia 1080 graphic card , it could used on other graphic card which support OpenCL and in other OS envirment. 

## Graphic card driver and OpenCl installation

You need a graphic card to start. We developed GPU mining module based on OenCL , and Nvidia 1080 and AMD Rx480, we also tested a group of other cards, like Nvidia 1060, 760, K2000 , and AMD Rx 580 etc.

But, not all GPU cards are supported by Ubuntu system, and how to install the GPU ubuntu drivers and OpenCl also varied.

We setup GPU card and install opencl as below steps, but it may not fit your situation, you may need find better way to resolve issues meet.

Configure
make sure that your user account is a member of the "video" group prior to using the driver. You can find which groups you are a member of with the following command:

    $ groups
To add yourself to the video group you will need the sudo password and can use the following command:

   $ sudo usermod -a -G video $LOGNAME 
You will need to log out and in again to activate this change.

Install graphic card driver and OpenCL
OpenCL support comes with the graphic card driver. Please check your graphic card vendor website, and found out how to install your graphic card driver and Open Cl.
#### Ubuntu 16.04 / amdgpu
 
install amdgpu-pro and AMD APP SDK.  reference https://gist.github.com/daveselinger/8cba6d41eaa70b220725091390ff52c1

 #### Nvidia

 please refer  install-proprietary-nvidia-gpu-drivers-on-ubuntu-16-04 , you need do more research to find out what's the best version for your graphic card. 


    $sudo apt-get install nvidia-opencl-dev 
    $sudo apt-get install nvidia-361

   
 ### GPU driver and opencl check 

    #check hardware installed, you should see your device below
    $ lspci |grep VGA   

    #check opencl, you should see OpenCL information below.
    $ clinfo 

    #check GPU status (for Nvidia) , you will see your graphic card status regardless fabcoin system.
    $ nvidia-smi 


 Run Fabcoin GPU mining 

    $cd ~/fabcoin/bin
    $./fabcoind -daemon -gen -G -allgpu 

    # check you GPU status, you should see GPU is busy.
    $nvidia-smi

    # check debug.log, you should see "FabcoinMiner GPU platform=0 " information
    $tail -f ~/.fabcoin/debug.log
  
## Compilation and installation Fabcoin code

Compile and make fabcoin with option --enable-gpu, gpu mining is default disable on makefile.

    cd ~/fabcoin
    ./autogen.sh
    ./configure --enable-gpu
    make 

 
### Run Mining  
call fabcoind or fabcoin-qt  with option -gen  -G -allgpu will start GPU mining 
  
    $ fabcoind \
         -gen \       # -gen will enable mining process. without -G , it will using CPU mining .
         -genproclimit  # Set threads numbers for CPU mining  (-1 = all cores, default 1) .
         -G   \         # -G , Enable GPU mining  (default: false, don't use GPU)
         -device=<id> \ # If -G is enabled this specifies the GPU device number to use(default: 0)
         -allgpu        # If -G is enabled this will mine on all available GPU devices (default: false)
 
example :

     $ fabcoind -testnet -daemon -gen -G -allgpu      # start GPU mining on all GPU card on testnet.
 
