# Libraries
# BLIS
BLIS_PREFIX = /usr/local
BLIS_INC    = $(BLIS_PREFIX)/include/blis
BLIS_LIB    = $(BLIS_PREFIX)/lib/libblis.a

# MKL
MKL_PREFIX = /opt/intel
MKL_INC    = $(MKL_PREFIX)/mkl/include
MKL_LIB    = $(MKL_PREFIX)/mkl/lib/intel64

#OpenBLAS
OPENBLAS_PREFIX = /usr/include
OPENBLAS_INC = $(OPENBLAS_PREFIX)/openblas

# Model / Tokenizer Paths
MOD_PATH    = out/model.bin
TOK_PATH    = tokenizer.bin

# choose your compiler, e.g. gcc/clang
# example override to clang: make run CC=clang

CC = gcc

#LLVM BOLT
#Set required linker flags
BOLT :=
ifeq ($(BOLTPREP),1)
  ifeq ($(CC),gcc)
    BOLT := -fno-reorder-blocks-and-partition -Wl,--emit-relocs
  else
    BOLT := -Wl,--emit-relocs
  endif
endif

##@ Simple Builds
# the most basic way of building that is most likely to work on most systems
.PHONY: run
run: run_cc	

.PHONY: runq
runq: runq_cc	

.PHONY: run_cc
run_cc: ##		- Standard build with basic optimizations
	$(CC) -O3 -march=native -mtune=native -o run run.c $(BOLT) -lm 

.PHONY: runq_cc
runq_cc: ##		- Same for quantized build
	$(CC) -O3 -march=native -mtune=native -o run runq.c $(BOLT) -lm 

# https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
# https://simonbyrne.github.io/notes/fastmath/
# -Ofast enables all -O3 optimizations.
# Disregards strict standards compliance.
# It also enables optimizations that are not valid for all standard-compliant programs.
# It turns on -ffast-math, -fallow-store-data-races and the Fortran-specific
# -fstack-arrays, unless -fmax-stack-var-size is specified, and -fno-protect-parens.
# It turns off -fsemantic-interposition.
# In our specific application this is *probably* okay to use
.PHONY: run_cc_fast
run_cc_fast: ##		- More Optimized build. Disregards strict standards compliance
	$(CC) -Ofast -march=native -mtune=native -o run run.c $(BOLT) -lm 

.PHONY: runq_cc_fast
runq_cc_fast: ##		- Same for quantized build
	$(CC) -Ofast -march=native -mtune=native -o run runq.c $(BOLT) -lm 

# compiles with gnu99 standard flags for amazon linux, coreos, etc. compatibility
.PHONY: run_cc_gnu
run_cc_gnu: ##		- Optimized Generic linux distro build
	$(CC) -Ofast -march=native -mtune=native -std=gnu11 -o run run.c $(BOLT) -lm 

.PHONY: runq_cc_gnu
runq_cc_gnu: ##		- Same for quantized build
	$(CC) -Ofast -march=native -mtune=native -std=gnu11 -o run runq.c $(BOLT) -lm 

##@ Accelerated Builds
# additionally compiles with OpenMP, allowing multithreaded runs
# make sure to also enable multiple threads when running, e.g.:
# OMP_NUM_THREADS=4 ./run out/model.bin

.PHONY: run_cc_avx
run_cc_avx: ##		- ***NEW*** AVX accelerated build
	$(CC) -D OPENMP -D ACCELAVX -Ofast -fopenmp -mavx -march=native -mtune=native run.c $(BOLT) -lm  -o run

.PHONY: run_cc_openmp
run_cc_openmp: ##		- OpenMP accelerated build
	$(CC) -D OPENMP -Ofast -fopenmp -march=native -mtune=native run.c  $(BOLT) -lm  -o run

.PHONY: runq_cc_openmp
runq_cc_openmp: ##		- Same for quantized build
	$(CC) -D OPENMP -Ofast -fopenmp -march=native -mtune=native runq.c  $(BOLT) -lm  -o run

.PHONY: run_cc_openacc
run_cc_openacc: ##		- OpenACC accelerated build
	$(CC) -D OPENACC -Ofast -fopenacc -march=native -mtune=native run.c  $(BOLT) -lm  -o run	

.PHONY: runq_cc_openacc
runq_cc_openacc: ##		- Same for quantized build
	$(CC) -D OPENACC -Ofast -fopenacc -march=native -mtune=native runq.c  $(BOLT) -lm  -o run	

.PHONY: run_cc_omp_gnu
run_cc_omp_gnu: ##		- Generic linux distro + OpenMP build
	$(CC) -D OPENMP -Ofast -fopenmp -march=native -mtune=native -std=gnu11 run.c  $(BOLT) -lm  -o run

.PHONY: runq_cc_omp_gnu
runq_cc_omp_gnu: ##		- Same for quantized build
	$(CC) -D OPENMP -Ofast -fopenmp -march=native -mtune=native -std=gnu11 runq.c  $(BOLT) -lm  -o run

.PHONY: run_cc_clblast
run_cc_clblast: ##		- CLBlast OpenCL CBLAS GPU accelerated build
	$(CC) -D OPENMP -D CLBLAST -Ofast -fopenmp -march=native -mtune=native run.c $(BOLT) -lm -lclblast -o run

.PHONY: runq_cc_clblast
runq_cc_clblast: ##		- Same for quantized build
	$(CC) -D OPENMP -D CLBLAST -Ofast -fopenmp -march=native -mtune=native runq.c $(BOLT) -lm -lclblast -o run

.PHONY: run_cc_openblas
run_cc_openblas: ##		- Openblas CBLAS accelerated build
	$(CC) -D OPENMP -D OPENBLAS -Ofast -fopenmp -march=native -mtune=native -I$(OPENBLAS_INC) run.c $(BOLT) -lm -lopenblas -o run

.PHONY: runq_cc_openblas
runq_cc_openblas: ##		- Same for quantized build
	$(CC) -D OPENMP -D OPENBLAS -Ofast -fopenmp -march=native -mtune=native -I$(OPENBLAS_INC) runq.c $(BOLT) -lm -lopenblas -o run

.PHONY: run_cc_cblas
run_cc_cblas: ##		- Generic CBLAS accelerated build
	$(CC) -D OPENMP -D CBLAS -Ofast -fopenmp -march=native -mtune=native run.c $(BOLT) -lm -lcblas -o run

.PHONY: runq_cc_cblas
runq_cc_cblas: ##		- Same for quantized build
	$(CC) -D OPENMP -D CBLAS -Ofast -fopenmp -march=native -mtune=native runq.c $(BOLT) -lm -lcblas -o run

.PHONY: run_cc_blis
run_cc_blis: ##		- BLIS accelerated build
	$(CC) -D OPENMP -D BLIS -Ofast -fopenmp -march=native -mtune=native -I$(BLIS_INC) run.c $(BOLT) -lm -lblis -o run
	
.PHONY: runq_cc_blis
runq_cc_blis: ##		- Same for quantized build
	$(CC) -D OPENMP -D BLIS -Ofast -fopenmp -march=native -mtune=native -I$(BLIS_INC) runq.c $(BOLT) -lm -lblis -o run

##@ Special Builds 
##@ ---> x86_64
# amd64 (x86_64) / Intel Mac 
.PHONY: run_cc_mkl 
run_cc_mkl: ##		- ***NEW*** OpenMP + Intel MKL CBLAS build (x86_64 / intel Mac)
	$(CC) -D MKL -D OPENMP -Ofast -fopenmp -march=native -mtune=native -I$(MKL_INC) -L$(MKL_LIB) run.c -lmkl_rt -lpthread $(BOLT) -lm -o run	

.PHONY: runq_cc_mkl 
runq_cc_mkl: ##		- Same for quantized build
	$(CC) -D MKL -D OPENMP -Ofast -fopenmp -march=native -mtune=native -I$(MKL_INC) -L$(MKL_LIB) runq.c -lmkl_rt -lpthread $(BOLT) -lm -o run	

##@ ---> ARM64 / aarch64
.PHONY: run_cc_armpl
run_cc_armpl: ##		- ARM PL BLAS accelerated build (aarch64)
	$(CC) -D ARMPL -D OPENMP -Ofast -fopenmp -march=native -mtune=native run.c $(BOLT) -lm -larmpl_lp64_mp -o run

.PHONY: runq_cc_armpl
runq_cc_armpl: ##		- Same for quantized build
	$(CC) -D ARMPL -D OPENMP -Ofast -fopenmp -march=native -mtune=native runq.c $(BOLT) -lm -larmpl_lp64_mp -o run

##@ ---> Macintosh
.PHONY: run_cc_mac_accel
run_cc_mac_accel: ##		- Mac OS OPENMP + CBLAS via Accelerate Framework build (WIP/TEST)
	$(CC) -D AAF -D OPENMP -Ofast -fopenmp -march=native -mtune=native run.c $(BOLT) -lm -framework Accelerate -o run

.PHONY: runq_cc_mac_accel
runq_cc_mac_accel: ##		- Same for quantized build
	$(CC) -D AAF -D OPENMP -Ofast -fopenmp -march=native -mtune=native runq.c $(BOLT) -lm -framework Accelerate -o run

##@ ---> Windows
.PHONY: run_win64
run_win: ##		- Optimized Windows build with MinGW-w64 toolchain
	x86_64-w64-mingw32-gcc -Ofast -march=native -mtune=native -D_WIN32 -o run.exe -I. run.c win.c

.PHONY: runq_win64
runq_win: ##		- Same for quantized build
	x86_64-w64-mingw32-gcc -Ofast -march=native -mtune=native -D_WIN32 -o run.exe -I. runq.c win.c

.PHONY: run_win_msvc
run_win_msvc: ##		- OpenMP accelerated Windows build with MSVC toolchain (Untested)
	cl.exe /fp:fast /Ox /DOPENMP /openmp /I. run.c win.c

.PHONY: runq_win_msvc
runq_win_msvc: ##		- Same for quantized build
	cl.exe /fp:fast /Ox /DOPENMP /openmp /I. runq.c win.c

##@ ---> MultiOS Builds (using cosmopolitan libc + toolchain)
# Cosmocc
.PHONY: run_cosmocc
run_cosmocc: ##		- Optimized Portable + cosmocc (runs on all OSes)
	cosmocc -Ofast -D COSMO_BLINK -D COSMO_METAL run.c $(BOLT) -lm -o run.com

.PHONY: runq_cosmocc
runq_cosmocc: ##		- Same for quantized build
	cosmocc -Ofast -D COSMO_BLINK -D COSMO_METAL runq.c $(BOLT) -lm -o run.com

##@ ---> MultiOS Builds ---> with Embedded Models
# Cosmocc + embedded model & tokenizer
.PHONY: run_cosmocc_zipos
run_cosmocc_zipos: ##		- Optimized Portable + cosmocc + embedded zip model build (runs on all OSes)
	cosmocc -Ofast -D COSMO_BLINK -D COSMO_METAL -D COSMO_ZIP run.c $(BOLT) -lm -o run.com
	zip run.com $(MOD_PATH)
	zip run.com $(TOK_PATH)

.PHONY: runq_cosmocc_zipos
runq_cosmocc_zipos: ##		- Same for quantized build
	cosmocc -Ofast -D COSMO_BLINK -D COSMO_METAL -D COSMO_ZIP runq.c $(BOLT) -lm -o run.com
	zip run.com $(MOD_PATH)
	zip run.com $(TOK_PATH)

.PHONY: run_cosmocc_incbin
run_cosmocc_incbin: ##		- Optimized Portable + cosmocc + embedded model fast build (runs on all OSes)
	cosmocc -Ofast -D COSMO_BLINK -D COSMO_METAL -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP run.c $(BOLT) -lm -o run.com

.PHONY: runq_cosmocc_incbin
runq_cosmocc_incbin: ##		- Same for quantized build
	cosmocc -Ofast -D COSMO_BLINK -D COSMO_METAL -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP runq.c $(BOLT) -lm -o run.com

.PHONY: run_cosmocc_strlit
run_cosmocc_strlit: ##		- Optimized Portable + cosmocc + embedded model build (runs on all OSes)
	gcc -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	cosmocc -Ofast -D COSMO_BLINK -D COSMO_METAL -D STRLIT -D LLOOP run.c $(BOLT) -lm -o run.com

.PHONY: runq_cosmocc_strlit
runq_cosmocc_strlit: ##		- Same for quantized build
	gcc -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	cosmocc -Ofast -D COSMO_BLINK -D COSMO_METAL -D STRLIT -D LLOOP runq.c $(BOLT) -lm -o run.com

##@ ---> GCC/Clang Embedded Model Builds
# GCC OpenMP + embedded model & tokenizer	
.PHONY: run_gcc_openmp_incbin
run_gcc_openmp_incbin: ##	- Gcc + OpenMP + embedded model fast build
	gcc -D OPENMP -Ofast -fopenmp -foffload-options="-Ofast $(BOLT) -lm" -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP -Wl,--emit-relocs run.c  $(BOLT) -lm  -o run	

.PHONY: runq_gcc_openmp_incbin
runq_gcc_openmp_incbin: ##	- Same for quantized build
	gcc -D OPENMP -Ofast -fopenmp -foffload-options="-Ofast $(BOLT) -lm" -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP runq.c  $(BOLT) -lm  -o run	

.PHONY: run_gcc_openmp_strlit
run_gcc_openmp_strlit: ##	- Gcc + OpenMP + embedded model build
	gcc -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	gcc -D OPENMP -Ofast -fopenmp -foffload-options="-Ofast $(BOLT) -lm" -march=native -mtune=native -D STRLIT -D LLOOP run.c  $(BOLT) -lm  -o run	

.PHONY: runq_gcc_openmp_strlit
runq_gcc_openmp_strlit: ##	- Same for quantized build
	gcc -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	gcc -D OPENMP -Ofast -fopenmp -foffload-options="-Ofast $(BOLT) -lm" -march=native -mtune=native -D STRLIT -D LLOOP runq.c  $(BOLT) -lm  -o run	

# Clang OpenMP + embedded model & tokenizer	
.PHONY: run_clang_openmp_incbin
run_clang_openmp_incbin: ##	- Clang + OpenMP + embedded model fast build
	clang -D OPENMP -Ofast -fopenmp -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP run.c  $(BOLT) -lm  -o run	

.PHONY: runq_clang_openmp_incbin
runq_clang_openmp_incbin: ##	- Same for quantized build
	clang -D OPENMP -Ofast -fopenmp -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP runq.c  $(BOLT) -lm  -o run	

.PHONY: run_clang_openmp_strlit
run_clang_openmp_strlit: ##	- Clang + OpenMP + embedded model build
	clang -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	clang -D OPENMP -Ofast -fopenmp -march=native -mtune=native -D STRLIT -D LLOOP run.c  $(BOLT) -lm  -o run		

.PHONY: runq_clang_openmp_strlit
runq_clang_openmp_strlit: ##	- Same for quantized build
	clang -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	clang -D OPENMP -Ofast -fopenmp -march=native -mtune=native -D STRLIT -D LLOOP runq.c  $(BOLT) -lm  -o run	

##@ ---> GCC/Clang Embedded Model Builds ---> Statically Linked
# GCC static + embedded model & tokenizer
.PHONY: run_gcc_static_incbin
run_gcc_static_incbin: ##	- Optimized Static gcc + embedded model fast build
	gcc -Ofast -static -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP run.c  $(BOLT) -lm  -o run	

.PHONY: runq_gcc_static_incbin
runq_gcc_static_incbin: ##	- Same for quantized build
	gcc -Ofast -static -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP runq.c  $(BOLT) -lm  -o run	

.PHONY: run_gcc_static_strlit
run_gcc_static_strlit: ##	- Optimized Static gcc + embedded model build
	gcc -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	gcc -Ofast -static -march=native -mtune=native -D STRLIT -D LLOOP run.c  $(BOLT) -lm  -o run

.PHONY: runq_gcc_static_strlit
runq_gcc_static_strlit: ##	- Same for quantized build
	gcc -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	gcc -Ofast -static -march=native -mtune=native -D STRLIT -D LLOOP runq.c  $(BOLT) -lm  -o run

# Clang static + embedded model & tokenizer
.PHONY: run_clang_static_incbin
run_clang_static_incbin: ##	- Optimized Static clang + embedded model fast build
	clang -Ofast -static -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP run.c  $(BOLT) -lm  -o run	

.PHONY: runq_clang_static_incbin
runq_clang_static_incbin: ##	- Same for quantized build
	clang -Ofast -static -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -D LLOOP runq.c  $(BOLT) -lm  -o run	

.PHONY: run_clang_static_strlit
run_clang_static_strlit: ##	- Optimized Static clang + embedded model build
	clang -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	clang -Ofast -static -march=native -mtune=native -D STRLIT -D LLOOP run.c  $(BOLT) -lm  -o run

.PHONY: runq_clang_static_strlit
runq_clang_static_strlit: ##	- Same for quantized build
	clang -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	clang -Ofast -static -march=native -mtune=native -D STRLIT -D LLOOP runq.c  $(BOLT) -lm  -o run

# Build for termux on Android
##@ ---> Android
.PHONY: run_incbin_tmux
run_incbin_tmux: get_model ##		- Optimized build + Embedded Model for Termux on Android
	$(CC) -Ofast -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -o run run.c $(BOLT) -lm 

.PHONY: runq_incbin_tmux
runq_incbin_tmux: get_model ##		- Same for quantized build
	$(CC) -Ofast -march=native -mtune=native -D INC_BIN -D MODPATH=$(MOD_PATH) -D TOKPATH=$(TOK_PATH) -o run runq.c $(BOLT) -lm 


##@ Post Compile Optimizations
##@ Binary Optimization 
# These are applied after the compile step and require respective tools. 
# -Wl,--emit-relocs option needed
.PHONY: run_bolt
run_bolt: ##		- ***NEW*** Apply llvm bolt binary optimization
	echo "NOTE: Needs llvm-bolt to be installed. Needs two passes, follow usage info."
	echo "Usage: make BOLTPREP=1 <target> ; make run_bolt"
	echo "Note: Not all targets are supported. Does not work without the first \"make BOLTPREP=1 <target>\" pass. Does not work for statically linked builds yet as well as any builds that have been stripped..."
	echo "Instrumenting..."

	if [ -f "run" ]; then echo "Optimizing run binary" ;\
	llvm-bolt run -instrument -o run.bolt.instrumentation ;\
	echo "Run the executable once to create profile data..." ;\
	echo "NOTE: For embedded model builds type a prompt, wait for response then type exit on next prompt..." ;\
	./run.bolt.instrumentation out/model.bin ;\
	echo "Create run.bolt optimized binary..." ;\
	llvm-bolt run -o run.bolt --use-gnu-stack -data=/tmp/prof.fdata -reorder-blocks=ext-tsp -reorder-functions=hfsort -split-functions -split-all-cold -split-eh -dyno-stats ;\
	fi

	if [ -f "runq" ]; then echo "Optimizing runq binary" ;\
	llvm-bolt runq -instrument -o runq.bolt.instrumentation ;\
	echo "Run the executable once to create profile data..." ;\
	echo "NOTE: For embedded model builds type a prompt, wait for response then type exit on next prompt..." ;\
	./run.bolt.instrumentation out/model.bin  ;\
	echo "Create runq.bolt optimized binary..." ;\
	llvm-bolt runq -o runq.bolt --use-gnu-stack -data=/tmp/prof.fdata -reorder-blocks=ext-tsp -reorder-functions=hfsort -split-functions -split-all-cold -split-eh -dyno-stats ;\
	fi

	echo "Done. Check out run.bolt or runq.bolt binaries."
	
##@ Strip Symbols 
# Stripping symbols make binaries small 
.PHONY: run_strip
run_strip: ##		- ***NEW*** Strip symbols to make binaries smaller
	strip -s run
	strip -s runq
	strip -s run.bolt
	strip -s runq.bolt

##@ ---> L2E Unikernel (Asteroid)	
# Unikraft Unikernel build
.PHONY: l2e_unik_qemu
l2e_unik_qemu: get_model ##		- L2E Unikernel (Asteroid) for kvm / qemu x86_64
	if [ ! -d "UNIK" ]; then echo "Cloning unikraft 0.14.0 and musl sources..." ; fi
	if [ ! -d "UNIK/unikraft" ]; then git clone -b RELEASE-0.14.0 --single-branch https://github.com/unikraft/unikraft UNIK/unikraft ; fi
	if [ ! -d "UNIK/libs/musl" ]; then git clone -b RELEASE-0.14.0 --single-branch https://github.com/unikraft/lib-musl UNIK/libs/musl ; fi
	UK_DEFCONFIG=$(shell pwd)/defconfigs/qemu-x86_64 make -f Makefile.unikernel defconfig
	$(CC) -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	make -f Makefile.unikernel
	
##@ ---> L2E Unikernel (Latest) (Asteroid)	
# Unikraft Unikernel latest version build
.PHONY: l2e_unik_qemu_latest
l2e_unik_qemu_latest: get_model ##		- L2E Unikernel (Latest unikraft unikernel) (Asteroid) for kvm / qemu x86_64
	if [ ! -d "UNIK" ]; then echo "Cloning latest unikraft and musl sources..." ; fi
	if [ ! -d "UNIK/unikraft" ]; then git clone https://github.com/unikraft/unikraft UNIK/unikraft ; fi
	if [ ! -d "UNIK/libs/musl" ]; then git clone https://github.com/unikraft/lib-musl UNIK/libs/musl ; fi	
	UK_DEFCONFIG=$(shell pwd)/defconfigs/qemu-x86_64 make -f Makefile.unikernel defconfig
	$(CC) -Ofast strliteral.c -o strlit
	./strlit -i emb_Model_data $(MOD_PATH) model.h
	./strlit -i emb_Tokenizer_data $(TOK_PATH) tokenizer.h
	make -f Makefile.unikernel

##@ ---> L2E Unikernel (Asteroid) ---> Boot in qemu
.PHONY: boot_l2e_unik
boot_l2e_unik: ##		- Boot L2E Unikernel (Asteroid) in qemu
	qemu-system-x86_64 -m 512m --accel kvm  --kernel  build/L2E_qemu-x86_64 --serial stdio 		

##@ ---> L2E OS (Humanoid)
# L2E OS - OS based on linux kernel
.PHONY: l2e_os
l2e_os: get_model tempclean ##		- L2E OS, kernel module and userspace build
	if [ ! -d "l2e_boot/linux" ]; then echo "Cloning linux v6.5 sources..." ;\
	git clone -b v6.5 --single-branch https://github.com/torvalds/linux.git l2e_boot/linux ;\
	fi
	
	if [ ! -d "l2e_boot/musl" ]; then echo "Cloning musl v1.2.4 sources..." ;\
	git clone -b v1.2.4 --single-branch git://git.musl-libc.org/musl l2e_boot/musl ;\
	fi
	
	if [ ! -d "l2e_boot/toybox" ]; then echo "Cloning toybox 0.8.10 sources..." ;\
	git clone -b 0.8.10 --single-branch https://github.com/landley/toybox.git l2e_boot/toybox ;\
	fi
	
	if [ ! -d "l2e_boot/busybox" ]; then echo "Cloning busybox 1_36_stable (1.36.1) sources..." ;\
	git clone -b 1_36_stable --depth 1 https://git.busybox.net/busybox l2e_boot/busybox ;\
	fi

	if [ ! -d "l2e_boot/fbDOOM" ]; then echo "Cloning fbDOOM..." ;\
	git clone --depth 1 https://github.com/maximevince/fbDOOM.git l2e_boot/fbDOOM ;\
	echo "Downloading Freedoom 0.12.1 WADs" ;\
	cd l2e_boot/fbDOOM ;\
	if [ -x "`which jar 2>/dev/null`" ]; then curl -L https://github.com/freedoom/freedoom/releases/download/v0.12.1/freedm-0.12.1.zip --output - | jar -xv freedm-0.12.1/freedm.wad ;\
	curl -L https://github.com/freedoom/freedoom/releases/download/v0.12.1/freedoom-0.12.1.zip --output - | jar -xv freedoom-0.12.1/freedoom1.wad freedoom-0.12.1/freedoom2.wad ;\
	mv freedm-0.12.1/*.wad ./ ;\
	mv freedoom-0.12.1/*.wad ./ ;\
	rm -rf freedm-0.12.1 freedoom-0.12.1 ;\
	else wget https://github.com/freedoom/freedoom/releases/download/v0.12.1/freedm-0.12.1.zip ;\
	wget https://github.com/freedoom/freedoom/releases/download/v0.12.1/freedoom-0.12.1.zip ;\
	unzip -p freedm-0.12.1.zip freedm-0.12.1/freedm.wad > freedm.wad ;\
	unzip -p freedoom-0.12.1.zip freedoom-0.12.1/freedoom1.wad > freedoom1.wad ;\
	unzip -p freedoom-0.12.1.zip freedoom-0.12.1/freedoom2.wad > freedoom2.wad ;\
	rm freedm-0.12.1.zip freedoom-0.12.1.zip ;\
	fi;\
	fi
	
	if [ ! -d "l2e_boot/l2e_sources/l2eterm/lvgl" ]; then echo "Cloning LVGL: lvgl v8.3.9 sources..." ;\
	cd l2e_boot/l2e_sources/l2eterm ;\
	git clone -b v8.3.9 --depth 1 https://github.com/lvgl/lvgl.git ;\
	echo "Cloning LVGL: lv_drivers v8.3.0 sources..." ;\
	git clone -b v8.3.0 --depth 1 https://github.com/lvgl/lv_drivers.git ;\
	fi
	
	if [ ! -d "l2e_boot/limine" ]; then echo "Downloading & extracting limine v5.20230830.0 (tar.xz) sources..." ;\
	mkdir l2e_boot/limine ;\
	curl -L https://github.com/limine-bootloader/limine/releases/download/v5.20230830.0/limine-5.20230830.0.tar.xz --output - | tar -xvf - -J -C l2e_boot/limine --strip-components 1 ;\
	fi
	
	cp -R l2e_boot/l2e_sources/l2e l2e_boot/linux/
	cp -R l2e_boot/l2e_sources/l2e_os l2e_boot/linux/drivers/misc/
	cp l2e_boot/l2e_sources/Kconfig l2e_boot/linux/drivers/misc/
	cp l2e_boot/l2e_sources/Makefile l2e_boot/linux/drivers/misc/
	cp l2e_boot/l2e_sources/L2E.gcc.config l2e_boot/linux/.config
	cp run.c l2e_boot/linux/l2e/
	cp incbin.h l2e_boot/linux/l2e/
	cp tokenizer.bin l2e_boot/linux/l2e/
	cp out/model.bin l2e_boot/linux/l2e/
	
	if [ ! -d "l2e_boot/musl_build" ]; then cd l2e_boot/musl ;\
	./configure --disable-shared --prefix=../musl_build --syslibdir=../musl_build/lib ;\
	make ;\
	make install ;\
	cd ../musl_build ;\
	sed -i s@../musl_build@`pwd`@g bin/musl-gcc ;\
	sed -i s@../musl_build@`pwd`@g lib/musl-gcc.specs ;\
	cd ../linux ;\
	make headers_install INSTALL_HDR_PATH=../kernel_headers;\
	cp -r ../kernel_headers/include/linux ../musl_build/include/;\
	cp -r ../kernel_headers/include/asm ../musl_build/include/;\
	cp -r ../kernel_headers/include/asm-generic ../musl_build/include/;\
	cp -r ../kernel_headers/include/mtd ../musl_build/include/;\
	fi
	
	if [ ! -f "l2e_boot/linux/l2e/toybox" ]; then cd l2e_boot/toybox ;\
	cp ../l2e_sources/L2E.toybox.config .config ;\
	make CC=../musl_build/bin/musl-gcc CFLAGS="-static" ;\
	cp toybox ../linux/l2e/ ;\
	fi	

	if [ ! -f "l2e_boot/linux/l2e/busybox" ]; then cd l2e_boot/busybox ;\
	cp ../l2e_sources/L2E.busybox.config .config ;\
	KCONFIG_NOTIMESTAMP=1 make CC=../musl_build/bin/musl-gcc CFLAGS="-static" ;\
	cp busybox ../linux/l2e/ ;\
	fi

	if [ ! -f "l2e_boot/linux/l2e/fbdoom" ]; then cd l2e_boot/fbDOOM ;\
	cd fbdoom;\
	make clean ;\
	make CC=../../musl_build/bin/musl-gcc CFLAGS="-static" NOSDL=1 ;\
	strip -s fbdoom ;\
	cp fbdoom ../../linux/l2e/ ;\
	cp ../*.wad ../../linux/l2e/ ;\
	fi
	
	if [ ! -f "l2e_boot/linux/l2e/l2eterm" ]; then cd l2e_boot/l2e_sources/l2eterm ;\
	make clean ;\
	make CC=../../musl_build/bin/musl-gcc CFLAGS="-static" ;\
	cp l2eterm ../../linux/l2e/ ;\
	cp LAIRS.png ../../linux/l2e/ ;\
	fi	
	
	if [ ! -d "l2e_boot/limine/bin" ]; then cd l2e_boot/limine ;\
	./configure --enable-bios-cd --enable-bios-pxe --enable-bios --enable-uefi-x86-64 --enable-uefi-cd ;\
	make ;\
	rm -r ../ISO ;\
	cp -R ../l2e_sources/ISO ../ ;\
	cp bin/limine-bios-cd.bin ../ISO/ ;\
	cp bin/limine-bios.sys ../ISO/ ;\
	cp bin/limine-uefi-cd.bin ../ISO/ ;\
	cp bin/BOOTX64.EFI ../ISO/EFI/BOOT/ ;\
	fi
		
	cd l2e_boot/linux/l2e ; make l2e_bin_cc
	cd l2e_boot/linux ; SOURCE_DATE_EPOCH=1696185000 KBUILD_BUILD_TIMESTAMP="Oct 2 00:00:00 UTC 2023" KBUILD_BUILD_USER=Vulcan KBUILD_BUILD_HOST=amica.board make LOCALVERSION="- TEMPLE DOS"
	cp l2e_boot/linux/arch/x86/boot/bzImage l2e_boot/ISO/L2E_Exec

##@ ---> L2E OS (Humanoid) ---> Make Bootable ISO
.PHONY: l2e_os_iso
l2e_os_iso: l2e_os ##		- Make Bootable L2E OS Hybrid UEFI/BIOS ISO Image
	if [ -d 'l2e_boot/ISO' ]; then cd l2e_boot/ ;\
	ls  ;\
	rm *.iso ;\
	xorriso -volume_date uuid '2023100200000000' \
        -volume_date all_file_dates '2023100200000000' \
        -charset 'UTF-8' \
        -volid "L2E OS" \
        -volset_id "L2E OS v0.1 TEMPLE DOS" \
        -publisher "L2E OS @Vulcan Ignis @TRHolding @AMICA Board" \
        -application_id "L2E OS Live Boot Disk" \
        -copyright_file "COPYRIGHT.TXT" \
        -abstract_file "ABSTRACT.TXT" \
        -biblio_file 'BIBLIO.MARCXML' \
        -as mkisofs -b /limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot /limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        -preparer "L2E Build v0.1 + xorriso 1.5.6" \
        -sysid "L2E OS x86_64 UEFI/BIOS BOOT ISO" \
        ./ISO -o l2eos.iso ;\
        ./limine/bin/limine bios-install l2eos.iso ;\
        fi

##@ ---> L2E OS (Humanoid) ---> Boot in qemu
.PHONY: boot_l2e_os
boot_l2e_os: ##		- Boot L2E OS (Humanoid) in qemu
	qemu-system-x86_64 -display gtk,zoom-to-fit=off -m 512m -accel kvm  --kernel  l2e_boot/ISO/L2E_Exec -vga virtio --append "loglevel=1 vga=0x392" 	
	
##@ ---> L2E OS (Humanoid) ---> Boot ISO in qemu
.PHONY: boot_l2e_iso
boot_l2e_iso: ##		- Boot L2E OS ISO Image in qemu
	qemu-system-x86_64 -display gtk,zoom-to-fit=off -m 512 -accel kvm -vga virtio -cdrom l2e_boot/l2eos.iso
	
##@ ---> L2E OS (Humanoid) ---> Boot ISO with UEFI in qemu
.PHONY: boot_l2e_iso_uefi
boot_l2e_iso_uefi: ##		- Boot L2E OS ISO Image with UEFI in qemu
	qemu-system-x86_64 -display gtk,zoom-to-fit=off -bios /usr/share/ovmf/x64/OVMF.fd -m 512 -accel kvm  -vga virtio -cdrom l2e_boot/l2eos.iso	

##@ Debug Build

# useful for a debug build, can then e.g. analyze with valgrind, example:
# $ valgrind --leak-check=full ./run out/model.bin -n 3
.PHONY: run_debug
run_debug: ##		- Debug build which can be analyzed with tools like valgrind.
	$(CC) -g -o run run.c $(BOLT) -lm 

.PHONY: run_cc_bcdebug
run_cc_bcdebug: ##		- ***NEW*** Emit LLVM bitcode & transpile to C debug build
	echo "Requires clang-17, and llvm-cbe to be compiled and added to path."
	echo "Get llvm-cbe here: https://github.com/JuliaHubOSS/llvm-cbe"
	clang-17 -march=native -mtune=native  -S -emit-llvm -g run.c  
	llvm-cbe run.ll 
	$(CC) -Ofast -march=native -mtune=native -o run run.cbe.c $(BOLT) -lm 

.PHONY: runq_cc_bcdebug
runq_cc_bcdebug: ##		- Same for quantized build
	echo "Requires clang-17, and llvm-cbe to be compiled and added to path."
	echo "Get llvm-cbe here: https://github.com/JuliaHubOSS/llvm-cbe"
	clang-17 -march=native -mtune=native  -S -emit-llvm -g runq.c  
	llvm-cbe runq.ll 
	$(CC) -Ofast -march=native -mtune=native -o run runq.cbe.c $(BOLT) -lm 

.PHONY: run_cc_mmdebug
run_cc_mmdebug: ##		- ***NEW*** Matmul Debug Log build (Warning: Huge Logs)
	$(CC) -D MMDEBUG -Ofast -march=native -mtune=native run.c $(BOLT) -lm  -o run


##@ Testing
.PHONY: test
test: ##		- run all tests (inclusive python code, needs python)
	pytest

.PHONY: testc
testc: ##		- run only tests for run.c C implementation (needs python)
	pytest -k runc

# to increase verbosity level run e.g. as `make testcc VERBOSITY=1`
VERBOSITY ?= 0
.PHONY: testcc
testcc: ##		- run the C tests, without touching pytest / python
	$(CC) -DVERBOSITY=$(VERBOSITY) -O3 -o testc test.c $(BOLT) -lm
	./testc

##@ Clean/ Purge
.PHONY: tempclean
tempclean: ##		- Find and delete all temporary files left by editors  
	 find . -name '*~' 
	 find . -name '*~' -delete

.PHONY: clean
clean: ##		- Simple cleaning 
	rm -f run run.com model.h tokenizer.h strlit run.com.dbg *~ l2e_boot/linux/l2e/toybox l2e_boot/toybox/toybox l2e_boot/l2eos.iso
	cd l2e_boot/l2e_sources/l2e ; make clean
	if [ -d "l2e_boot/linux/l2e" ]; then cd l2e_boot/linux/l2e ; make clean ; fi
	if [ -d "l2e_boot/linux" ]; then cd l2e_boot/linux ; make clean ; fi	
	if [ -d "l2e_boot/toybox" ]; then cd l2e_boot/toybox ; make clean ; fi
	if [ -d "l2e_boot/busybox" ]; then cd l2e_boot/busybox ; make clean ; fi
	if [ -d "l2e_boot/l2e_sources/l2eterm/lvgl" ]; then cd l2e_boot/l2e_sources/l2eterm ; make clean ; fi
	if [ -d "l2e_boot/musl" ]; then cd l2e_boot/musl ; make clean ; fi
	if [ -d "l2e_boot/limine" ]; then cd l2e_boot/limine ; make clean ; fi
	if [ -d "l2e_boot/ISO" ]; then rm -r l2e_boot/ISO ; fi	
	if [ -d "build" ]; then make -f Makefile.unikernel clean ; fi
	
.PHONY: distclean
distclean: ##		- Deep cleaning (distclean sub projects)
	rm -f run run.com model.h tokenizer.h strlit run.com.dbg .config.old .config *~ l2e_boot/l2eos.iso
	cd l2e_boot/l2e_sources/l2e ; make clean	
	if [ -d "l2e_boot/linux/l2e" ]; then cd l2e_boot/linux/l2e ; make clean ; fi	
	if [ -d "l2e_boot/linux" ]; then cd l2e_boot/linux ; make distclean ; fi		
	if [ -d "l2e_boot/toybox" ]; then cd l2e_boot/toybox ; make distclean ; fi
	if [ -d "l2e_boot/busybox" ]; then cd l2e_boot/busybox ; make distclean ; fi
	if [ -d "l2e_boot/l2e_sources/l2eterm/lvgl" ]; then cd l2e_boot/l2e_sources/l2eterm ; make clean ; fi
	if [ -d "l2e_boot/musl" ]; then cd l2e_boot/musl ; make distclean ; fi
	if [ -d "l2e_boot/limine" ]; then cd l2e_boot/limine ; make distclean ; fi
	if [ -d "l2e_boot/ISO" ]; then rm -r l2e_boot/ISO ; fi		
	if [ -d "build" ]; then make -f Makefile.unikernel distclean ; fi
	rm -rf build
	rm -rf l2e_boot/musl_build
	rm -rf l2e_boot/kernel_headers
	
.PHONY:  mintclean
mintclean: ##		- Revert to mint condition (remove sub projects)
	rm -f run run.com model.h tokenizer.h strlit run.com.dbg .config.old .config *~ l2e_boot/l2eos.iso
	cd l2e_boot/l2e_sources/l2e ; make clean	
	if [ -d "l2e_boot/linux/l2e" ]; then cd l2e_boot/linux/l2e ; make clean ; fi	
	if [ -d "l2e_boot/linux" ]; then cd l2e_boot/linux ; make distclean ; fi			
	if [ -d "l2e_boot/toybox" ]; then cd l2e_boot/toybox ; make distclean ; fi
	if [ -d "l2e_boot/busybox" ]; then cd l2e_boot/busybox ; make distclean ; fi
	if [ -d "l2e_boot/l2e_sources/l2eterm/lvgl" ]; then cd l2e_boot/l2e_sources/l2eterm ; make clean ; fi
	if [ -d "l2e_boot/musl" ]; then cd l2e_boot/musl ; make distclean ; fi
	if [ -d "l2e_boot/limine" ]; then cd l2e_boot/limine ; make distclean ; fi	
	if [ -d "l2e_boot/ISO" ]; then rm -r l2e_boot/ISO ; fi		
	if [ -d "build" ]; then make -f Makefile.unikernel distclean ; fi
	rm -rf build
	rm -rf l2e_boot/musl_build
	rm -rf l2e_boot/kernel_headers	
	#rm -rf UNIK
	#rm -rf l2e_boot/musl
	#rm -rf l2e_boot/toybox
	#rm -rf l2e_boot/busybox
	#rm -rf l2e_boot/l2e_sources/l2eterm/lvgl
	#rm -rf l2e_boot/l2e_sources/l2eterm/lv_drivers	
	#rm -rf l2e_boot/limine
	#rm -rf l2e_boot/linux	

##@ Misc

.PHONY: get_model
get_model: ##		- Get stories15M model
	if [ ! -d "out/" ]; then echo "Downloading model..." ; fi
	if [ ! -d "out/" ]; then wget https://huggingface.co/karpathy/tinyllamas/resolve/main/stories15M.bin ; fi
	if [ ! -d "out/" ]; then mkdir out ; fi
	if [ -f "stories15M.bin" ]; then mv stories15M.bin out/model.bin ; fi

# Uses: https://stackoverflow.com/a/26339924 
.PHONY: list
list: ##		- Display sorted list of all targets
	@LC_ALL=C $(MAKE) -pRrq -f $(firstword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/(^|\n)# Files(\n|$$)/,/(^|\n) / {if ($$1 !~ "^[#.]") {print $$1}}' | sort | grep -E -v -e '^[^[:alnum:]]' -e '^$@$$'
	
##@ Help
# Credits:
# https://www.thapaliya.com/en/writings/well-documented-makefiles/
# https://gist.github.com/prwhite/8168133
# https://www.client9.com/self-documenting-makefiles/
.DEFAULT_GOAL=help
.PHONY=help	
help:  ##		- Display this help. Make without target also displays this help.
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make \033[36m<target>\033[0m\n"} /^[a-zA-Z0-9_-]+:.*?##/ { printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2 } /^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } ' $(MAKEFILE_LIST)
