# GOatLANG: Go-Compiler-and-Virtual-Machine

## Instruction to build the system
We simplify the instruction to build the system to the following:
1. `git clone https://github.com/jiasheng59/GOatLANG-Go-Compiler-and-Virtual-Machine.git`
2. `cd build`
3. `make`
4. `./GOatLANG`

To input Go program and run the test case, insert the GOatLANG program into `src/main.cpp` 's main function as argument of local variable `prog` see example as follow:


Follow the following instructions if you wish to build the entire system from scratch.
1. Go to https://www.antlr.org/download.html. Download `antlr4-cpp-runtime-4.13.1-source.zip`. Unzip `antlr4-cpp-runtime-4.7.2-source.zip` and cd to the resulting directory `antlr4-cpp-runtime-4.7.2-source` Make sure you have all the required tools. Type the following commands in the terminal to install:
  - `sudo apt install cmake`
  - `sudo apt install uuid-dev`
  - `sudo apt install pkg-config`
2. Type the following commands
   - `mkdir build && mkdir run && cd build`
   - `cmake ..`
  - `DESTDIR=../run make install`
  
3. Now copy the ANTLR 4 include files to /usr/local/include and the ANTLR 4 libraries to /usr/local/lib by entering the following commands (which assume that you’re still in the build
directory):
- `cd ../run/usr/local/include`
- `sudo cp -r antlr4-runtime /usr/local/include`
- `cd ../lib`
- `sudo cp * /usr/local/lib`
- `sudo ldconfig`

## Instruction on generating GOatLANG parser
How to generate GOatLANG parser?

```
mkdir build
cd build
cmake ..
make 
```

Parser will be generated into `generated/` directory.
