GCC_BIN=`xcrun --sdk iphoneos -f gcc`
GCC=$(GCC_BASE) -arch arm64 # -arch armv7s -arch armv7
SDK=`xcrun --sdk iphoneos --show-sdk-path`

CFLAGS = -Xlinker -iphoneos_version_min -Xlinker 6.0.0
GCC_BASE = $(GCC_BIN) -Os $(CFLAGS) -Wimplicit -isysroot $(SDK)

all: conf 

conf: conf.c
	$(GCC) -o $@ $^
# 	ldid -S $@

clean:
	rm -f *.o helloworld  


