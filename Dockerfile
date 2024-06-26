# Use a base image with C++ development tools installed
FROM gcc:latest

# Install CMake
RUN apt-get update && apt-get install -y cmake

# Install ANTLR4
RUN apt-get install -y antlr4

# Install ANTLR4 C++ runtime
RUN apt-get install -y libantlr4-runtime-dev

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY . .

# Create a directory for ANTLR4 runtime headers and copy them
RUN mkdir antlr4-runtime
COPY ./antlr4-runtime-copy/. /app/antlr4-runtime/.
COPY ./generated/.  /app/generated/

#######
COPY ./antlr4-runtime-copy/atn/. /app/antlr4-runtime/atn/.
COPY ./antlr4-runtime-copy/dfa/. /app/antlr4-runtime/dfa/.
COPY ./antlr4-runtime-copy/internal/. /app/antlr4-runtime/internal/.
COPY ./antlr4-runtime-copy/misc/. /app/antlr4-runtime/misc/.
COPY ./antlr4-runtime-copy/support/. /app/antlr4-runtime/support/.
COPY ./antlr4-runtime-copy/tree/. /app/antlr4-runtime/tree/.
COPY ./antlr4-runtime-copy/tree/pattern/. /app/antlr4-runtime/tree/pattern/.
COPY ./antlr4-runtime-copy/tree/xpath/. /app/antlr4-runtime/tree/xpath/.

#######


COPY libantlr4-runtime.a app/libantlr4-runtime.a

# Create the build directory
RUN mkdir build

# Switch to the build directory
WORKDIR /app/build

# Run CMake with the appropriate include path for ANTLR4 runtime headers
# RUN cmake -DANTLR4_INCLUDE_DIR=/app/antlr4-runtime ..
RUN cmake ..

# Compile the C++ code
RUN make

# Go back to the original working directory
WORKDIR /app

# Specify the default command to run when the container starts
CMD ["./build/GOatLANG"]
