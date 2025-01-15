## Llama 2 Everywhere (L2E)

<p align="center">
  <img src="assets/l2e_sky_fun.png" width="600" height="600" alt="LLamas Everywhere!">
</p>

Note: Much gratitude for all the upvotes on HN, Twitter, Reddit and other places. Check out my X for Amiga, Atari, Vintage Mac ports...

Standalone, Portable, Bootable Llama 2

The primary objective of Llama 2 Everywhere (L2E) is to ensure its compatibility across a wide range of devices, from booting on repurposed chromebooks discarded by school districts to high-density unikernel deployments in enterprises. 

We believe that in future by harnessing a legion of small specialized LLMs with modest hardware requirements which are networked, distributed, and self-coordinated, L2E has the potential to democratize access to AI and unlock collective intelligence that surpasses that of a single large LLM.

The current compelling use case of L2E involves training small models on diverse textual sources, including textbooks, open books, and comprehensive corpora like the SlimPajama corpus. These trained models can in future be deployed using L2E, enabling them to run as bootable instances on resource constrained computers. This deployment scenario proves particularly valuable in school libraries or classrooms where internet connectivity is limited or unavailable, serving as an information gateway* for students without constant reliance on the internet.

By pursuing the vision of Llama 2 Everywhere, we aim to create an inclusive AI ecosystem that can adapt to diverse environments and empower individuals and communities on a global scale.

Apart from that, my research goal is to train models using various hardware telemetry data with the hope that the models learn to interpret sensor inputs and control actuators based on the insights they glean from the sensor inputs. This research direction may open up exciting possibilities in fields such as automation, space, robotics and IoT, where L2E can play a pivotal role in bridging the gap between AI and physical systems.

Initially a friendly fork of the excellent [@karpathy's llama2.c](https://github.com/karpathy/llama2.c), by now highly diverged, considering that we can build a whole OS with this.

I will be merging the progress of https://github.com/karpathy/llama2.c once in a while, add portability, performance improvements and convenience features which certainly would not fit in the upstream do to the minimalistic elegance requirements there.

Learn more about the Llama2 models & architecture at Meta: [Llama 2 @ Meta](https://llama.meta.com/llama2/)

# What people say:
* [Andrej Karpathy on X](https://twitter.com/karpathy/status/1710061549677613469)

* [Once upon a time on Reddit](https://www.reddit.com/r/LocalLLaMA/comments/16zklam/i_created_an_os_that_boots_to_a_baby_llama2/?sort=top)

* [Once upon a time on HN](https://news.ycombinator.com/item?id=37785442)

# Features & Milestones

#### Llama 3.1 Support WIP

* Inference is ~23% faster now. (Commit e842bf7 and above)
* Buggy, read the Llama3 Section below, looking for faster hardware for faster development

Sample output:

Meta's Llama 3.1 models can output multilingual text which is awesome. Here are some examples output of 8 bit quantized 8b model with 100 token output (-n 100)... 

##### English

```
./run ../llama3.1_8b_instruct_q8.bin -z tokenizer_l3.bin -l 3 -n 100 -i " My cat is funny"
My cat is funny. "Funny cat," I say, walking up to it. "What are you up to?" It sits up straight and looks at me with a tilted head, as if to say, "What's wrong with you?" Sometimes I just have to laugh at how funny a cat can be. So I say, "Okay, you're funny. I'll give you some treats." It stretches out a little and I give it some treats. It eats them up quickly and starts
achieved tok/s: 5.376052
```

##### German

```
./run ../llama3.1_8b_instruct_q8.bin -z tokenizer_l3.bin -l 3 -n 100 -i " Besitzen Sie einen Amiga 500?"
Besitzen Sie einen Amiga 500? Wenn nicht, werden Sie wissen, dass dies ein Computer war, der im späten 1980er und frühen 1990er Jahren für Spiele verfügbar war, die für Personen mit bestimmten Körperverletzungen gedacht waren. Manchmal müssen wir uns an frühere Zeiten erinnern, die, wie wir jetzt wissen, schwierig waren. Hier ist ein Link, der meine Geschichte bespre
achieved tok/s: 5.367599

```

##### French


```
./run ../llama3.1_8b_instruct_q8.bin -z tokenizer_l3.bin -l 3 -n 100 -i " Le vin français est"
Le vin français est, à bien des égards, un vin des origines, car il a joué un rôle important dans l'histoire de la France". La réputation des vins de France repose principalement sur leurs qualités gustatives et la gestion des vignobles contrôlée, ce qui rend le vin français un "produit d'exception". La France est donc leader mondial de la production de vin, avec 25 % des exportations mon
achieved tok/s: 5.43299
```

##### Thai

```
./run ../llama3.1_8b_instruct_q8.bin -z tokenizer_l3.bin -l 3 -n 100 -i " แมวของฉันตลก"
แมวของฉันตลกชอบเล่นบนม้วนกระดาษ และฉันก็ไม่แน่ใจว่าควรจะยินยอมที่จะให้เล่นหรือไม่

เมื่อเวลาผ่านไป ฉันเห็นว่าแมวของฉันเล่นม้วนกระดาษเป็นระยะ ๆ ฉันจึงตัดสินใจที่จะลองปรับเปลี่ยนเกมให้สนุกขึ้น
achieved tok/s: 5.376052
```

##### Hindi

```
./run ../llama3.1_8b_instruct_q8.bin -z tokenizer_l3.bin -l 3 -n 100 -i " मेरी बिल्ली बहुत मज़ाया है"
मेरी बिल्ली बहुत मज़ाया है और वह हमेशा अपनी शारीरिक गतिविधियों से मुझे मजाक करती है। वास्तव में, जब वह अपनी खिलौनों की चपपेट में आती है तो वह विशेष रूप से क्लासिक बन जाती है। इसके अलावा, वह एक छोटी सी च
achieved tok/s: 5.460864
```

Read the Llama 3 section below to understand how to get access to model (https://huggingface.co/meta-llama/Meta-Llama-3.1-8B-Instruct) from Meta, and follow this:

```bash
huggingface-cli download meta-llama/Meta-Llama-3.1-8B-Instruct --include "original/*" --local-dir Meta-Llama-3.1-8B-Instruct

git clone https://github.com/trholding/llama2.c.git

cd llama2.c/

# Export Quantized 8bit 
python3 export.py ../llama3.1_8b_instruct_q8.bin --version 2 --meta-llama ../Meta-Llama-3.1-8B-Instruct/original/

# Fastest Quantized Inference build
make runq_cc_openmp

# Test Llama 3.1 inference, it should generate sensible text
./run ../llama3.1_8b_instruct_q8.bin -z tokenizer_l3.bin -l 3 -i " My cat"

```

#### Llama 3 Support WIP

Llama3 models work now.

* Non quantized (fp32) is supported. run supports both llama2 and llama3 with -l 3 option.
* Quantized inference with runq supported now.
* Known issues - Swallows first token (add space for now), chat mode doesn't work yet, fix coming soonish
* Overall buggy for now


Sample output:

```
./run ../llama3_8b_instruct_q8.bin -z tokenizer_l3.bin -l 3 -i " My cat"
My cat's got a whole lot of livin' to do!" She walked out, leaving me with the blank look of a drunk who'd just had a song stuck in his head. I stared after her, feeling like I was trapped in a surreal episode of "The Twilight Zone."

As I turned back to the bar, I spotted a familiar figure at the end of the counter. It was Mitch, the bartender, polishing a mug with a dirty rag. I slid onto the stool beside him and said, "That's one strange lady, Mitch."

Mitch looked up and raised an eyebrow. "You're telling me. She's been in here a few times, always ordering weird drinks and singing along to her own personal soundtrack. I think she's got a tape playing in her head and she's trying to sing along."

I laughed. "I think you're right. She's like the 21st-century equivalent of that crazy lady who used to sing 'My Way' at the piano in the department store."

Mitch chuckled. "Yeah, only instead of 'My Way,' she's got a cat with a whole lot of livin' to do."

I clinked my glass against his. "To the strange and wonderful patrons of this fine establishment."


achieved tok/s: 4.356963
```


First you'll need to obtain approval from Meta to download llama3 models on hugging face.

So go to https://huggingface.co/meta-llama/Meta-Llama-3-8B-Instruct, fill the form and then
go to https://huggingface.co/settings/gated-repos see acceptance status. Once accepted, do the following to download model, export and run. 

```bash
huggingface-cli download meta-llama/Meta-Llama-3-8B-Instruct --include "original/*" --local-dir Meta-Llama-3-8B-Instruct

git clone https://github.com/trholding/llama2.c.git

cd llama2.c/

# Export fp32
#python3 export.py ../llama3_8b_instruct.bin --meta-llama ../Meta-Llama-3-8B-Instruct/original/

# Export Quantized 8bit 
python3 export.py ../llama3_8b_instruct_q8.bin --version 2 --meta-llama ../Meta-Llama-3-8B-Instruct/original/

make runq_cc_openmp
# or do make to see all builds 

# Test Llama 3 inference, it should generate sensible text
 ./run ../llama3_8b_instruct_q8.bin -z tokenizer_l3.bin -l 3 -i " My cat"

```

Export should take about 10-15 minutes. But on slow systems or without enough RAM, you will need to add a swapfile (which you can later swapoff and delete). Export with swap could take much longer, like an hour or more for example on an oracle cloud aarch64 instance with 24GB RAM and 4 vCPUs it took more than an hour. This is how you enable swap:

```bash
sudo fallocate -l 32G swapfile
sudo chmod 600 swapfile
sudo mkswap swapfile 
sudo swapon swapfile
```

#### L2E OS (Linux Kernel)

Have you ever wanted to really boot and inference a baby Llama 2 model on a computer? No? Well, now you can!

![temple dos](https://github.com/trholding/llama2.c/assets/93451215/5142b54e-b46f-4224-99a6-44b8896d2358)

![GUI](https://github.com/trholding/llama2.c/assets/93451215/ce139c79-407d-4946-8244-0f38a3c07211)

Have you ever wanted to do `cat /dev/llama` and `echo "Sudo make me a sandwich!" > /dev/llama` or pass a kernel parameter such as `l2e.quest="What is the meaning of life?"` ? No? Well, as luck would have, it now you can!

![dmesg](https://github.com/trholding/llama2.c/assets/93451215/93b7e87b-170e-4a25-8b61-2a9cef461714)

Download and run the ISO from the latest release!

### Unikraft Unikernel Build

Have you ever wanted to boot and inference a herd of 1000's of Virtual baby Llama 2 models on big ass enterprise servers? No? Well, now you can!

![l2e_unik](https://github.com/trholding/llama2.c/assets/93451215/415f00b4-25ed-4c30-b619-1c3404ababee)


The cool folks at [Unikraft](https://unikraft.org/) / [Kraft Cloud](https://kraft.cloud/) have a offical unikraft fork of L2E unikraft unikernel here: [app-llama2-c](https://github.com/unikraft/app-llama2-c) be sure to try it out and support them by purchasing cloud accounts. Thank you @razvand & [@felipehuici](https://twitter.com/felipehuici) for making this possible.


We love unikraft so much that the future versions the L2E OS (linux) will have KVM and selfhosting support and demo instances of L2E unikraft unikernels so that you can test drive unikraft unikernels on your own machine!

Just do the following to build:

```bash
make run_unik_qemu_x86_64
```

Please note that the requirements - unikraft and musl sources - will automatically get cloned before building.

Once the build completes, (takes a while), run L2E like this:

```bash
qemu-system-x86_64 -m 256m -accel kvm -kernel build/L2E_qemu-x86_64
```

You can also run with -nographic option to directly interact in terminal.

```bash
qemu-system-x86_64 -m 256m -accel kvm -kernel build/L2E_qemu-x86_64 -nographic
```
Download and try this and the cosmocc build in the latest release.

## Portability Features

+ Single Executable that runs on any x86_64 OS (cosmocc builds)
- [x] GNU Linux 
- [x] GNU/Systemd
- [x] *BSD (NetBSD, OpenBSD, FreeBSD)
- [x] XNU's Not UNIX (Mac)
- [x] Bare Metal Boot (BIOS & EFI) (Not fully functional yet but almost...)
- [x] Windows
- [x] Runs on ARM64 via inbuilt BLINK emulation

+ Standalone
- [x] Embedded model and tokenizer via ZipOS (cosmocc), INCBIN, strliteral

+ Usability
- [x] Hacky CLI Chat - use any _incbin, _strlit or _zipos build.

Some combined features depend on a specific cosmocc toolchain: https://github.com/jart/cosmopolitan

Building this with gcc or clang would result in normal binaries similar to upstream.

Read more:
[How to build](https://github.com/trholding/llama2.c#portable-binary-build)

### Performance Features

**CPU**

- [x] OpenBLAS
- [x] CBLAS
- [x] BLIS
- [x] Intel MKL 
- [x] ArmPL
- [ ] Apple Accelerate Framework (CBLAS) (WIP/Testing)

**CPU/GPU**

- [x] OpenMP 
- [x] OpenACC

Both OpenMP and OpenACC builds currently use host CPU and do not offload to GPU.

**GPU**

- [x] OpenCL (via CLBlast) (Direct - planned)
- [ ] OpenGL 
- [ ] Vulkan 

Download the prebuilt run.com binary from releases

## llama2.c

<p align="left">
  <img src="assets/llama_cute.jpg" width="150" height="150" alt="Cute Llama">
</p>

A friendly fork of the excellent [llama2.c](https://github.com/karpathy/llama2.c)

The original repository offers a full-stack solution for training and inferring the Llama 2 LLM architecture using PyTorch and a simple 500-line C file. The focus is on minimalism and simplicity, and the repo is a young project that is still being actively developed. The author recommends looking at the TinyStories paper for inspiration, as small LLMs can have strong performance in narrow domains. The C inference engine in run.c was the main focus of the project, and the Llama 2 architecture is hard-coded with no dependencies.

## Feel the Magic

```bash
git clone https://github.com/trholding/llama2.c.git
cd llama2.c
make run_cc_fast
wget https://huggingface.co/karpathy/tinyllamas/resolve/main/stories15M.bin
./run stories15M.bin
```
You can also prompt the model with a prefix:

```bash
./run stories42M.bin -t 0.8 -n 256 -i "A big dog"
```

When prompting, the temperature and steps parameters are needed since we use simple positional arguments.

**Output**

> A big dog named Zip. He loved to run and play in the sun. He was a happy dog. One day, Zip saw a little bird on the ground. The bird looked helpless. Zip wanted to help the bird. He ran to the bird and lay down next to it. Zip and the bird became friends. They played together every day. Zip would carry the bird to play in the trees. The bird would fly around, and Zip would bark. They were very happy together.


## Models

The original author trained a series of small models on TinyStories, which took a few hours to train on their setup. The 110M model took around 24 hours. The models are hosted on huggingface hub:

| model | dim | n_layers | n_heads | max context length | parameters | val loss | download
| --- | --- | --- | --- | --- | --- | --- | --- |
| OG | 288 | 6 | 6 | 256 | 15M | 1.072 | [stories15M.bin](https://huggingface.co/karpathy/tinyllamas/resolve/main/stories15M.bin) |
| 42M| 512 | 8 | 8 | 1024 | 42M | 0.847 | [stories42M.bin](https://huggingface.co/karpathy/tinyllamas/resolve/main/stories42M.bin) |
| 110M| 768 | 12 | 12 | 1024 | 110M | 0.760 | [stories110M.bin](https://huggingface.co/karpathy/tinyllamas/resolve/main/stories110M.bin) |

The upstream project owner trained the llama2.c storyteller models on a 4X A100 40GB box provided by [Lambda labs](https://lambdalabs.com/service/gpu-cloud).

Quick note on sampling, the recommendation for good results is to use `-t 1.0 -p 0.9`, i.e. top-p sampling at 0.9 with temperature 1.0 (this is the default). To control the diversity of samples use either the temperature (i.e. vary `-t` between 0 and 1 and keep top-p off with `-p 0`) or the top-p value (i.e. vary `-p` between 0 and 1 and keep `-t 1`), but not both. Nice explainers on LLM sampling strategies include [this](https://peterchng.com/blog/2023/05/02/token-selection-strategies-top-k-top-p-and-temperature/), [this](https://docs.cohere.com/docs/controlling-generation-with-top-k-top-p) or [this](https://huggingface.co/blog/how-to-generate).

```bash
./run llama2_7b.bin
```
A converted **Meta's Llama 2 7b** model can be inferenced at a slow speed.

## Usage

**Full Usage**

```
Usage:   run <checkpoint> [options]
Example: run model.bin -n 256 -i "Once upon a time"
Options:
  -t <float>  temperature in [0,inf], default 1.0
  -p <float>  p value in top-p (nucleus) sampling in [0,1] default 0.9
  -s <int>    random seed, default time(NULL)
  -n <int>    number of steps to run for, default 256. 0 = max_seq_len
  -i <string> input prompt
  -z <string> optional path to custom tokenizer
  -m <string> mode: generate|chat, default: generate
  -y <string> (optional) system prompt in chat mode
  -b <int>    number of tokens to buffer, default 1. 0 = max_seq_len
  -x <int>    extended info / stats, default 1 = on. 0 = off

```
``<checkpoint>`` is the **mandatory** checkpoint / model file.

**Minimal Usage**

```bash
./run <checkpoint_file>
```

## Platforms

**Multi OS build**

`make run_cosmocc`

The binary will boot on baremetal and also run on any 64 Bit OS such as Linux, *BSD, Windows and slower on Aarch64 Mac & Linux.

Currently when used to boot, it won't be able to find the models. It's a toolchain feature with an anticipated PR merge.

The performance of this build is more than twice of the basic build.

The cosmopolitan toolchain is a requirement for this build to work. Read here: [How to build](https://github.com/trholding/llama2.c#portable-binary-build)

__Please note that the Multi OS binaries builds built with cosmocc cause a false positive with AV/Microsoft Defender and Virus Total__

The issue is that AV's consider unsigned binaries or binaries that contain multiple OS binary signatures in one binary as suspicious.
Get more insight here: https://github.com/trholding/llama2.c/issues/8 and https://github.com/jart/cosmopolitan/issues/342

**Linux**

Centos 7 / Amazon Linux 2018

`make rungnu` or `make runompgnu` to use openmp.

**Other Linux Distros / Mac**

`make runfast` or `make runomp` to use openmp.

**Windows**

Build on windows: 

`build_msvc.bat` in a Visual Studio Command Prompt 

The MSVC build will use openmp and max threads suitable for your CPU unless you set `OMP_NUM_THREADS` env.

Build on Linux and Windows:

`make win64` to use the mingw compiler toolchain.

**Android**

See this @lordunix's post on how to build this on Android within [termux](https://termux.dev/en/):

https://github.com/trholding/llama2.c/issues/7#issue-1867639275

TODO. 

## Performance

**Basic**

This build does not enable any optimizations.

```bash
make run
```
This can be used as baseline build against which performance of other builds can be compared.

**Fast**

This build enables basic performance boost with compiler provided optimizations.

```bash
make runfast
```
### Build wth Acceleration

**OpenMP**

This build enables acceleration via OpenMP

```bash
make run_cc_openmp
```

Requires [OpenMP](https://www.openmp.org/) libraries and compiler with OpenMP support to be available on system.
E.g. `apt install clang libomp-dev` on ubuntu

When you run inference make sure to use OpenMP flags to set the number of threads, e.g.:

```bash
OMP_NUM_THREADS=4 ./run out/model.bin
```
More threads is not always better.

**OpenACC**

This build enables acceleration via OpenACC

```bash
make run_cc_openacc
```

Requires [OpenACC](https://www.openacc.org/) libraries and compiler with OpenACC support to be available on system.

**OpenBLAS**

This build enables acceleration via OpenBLAS

```bash
make run_cc_openblas
```

Requires [OpenBLAS](https://github.com/xianyi/OpenBLAS) to be installed on system.

**BLIS**

This build enables acceleration via BLIS

```bash
make run_cc_blis
```
Requires [BLIS](https://github.com/flame/blis) compiled with `./configure --enable-cblas -t openmp,pthreads auto` to be installed on system.

**Intel oneAPI MKL**

This build enables acceleration via Intel® oneAPI Math Kernel Library on x86_64 systems and Intel Mac OS - WIP

```bash
make run_cc_mkl
```
Requires [Intel oneAPI MKL](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl.html) to be installed on system.

**Arm Performance Library (ArmPL)**

This build enables acceleration via Arm Performance Library on ARM64 systems such as Linux or Mac OS 

First you'll need to download ArmPL and install it:

```bash
wget https://developer.arm.com/-/cdn-downloads/permalink/Arm-Performance-Libraries/Version_24.04/arm-performance-libraries_24.04_deb_gcc.tar

tar -xvf arm-performance-libraries_24.04_deb_gcc.tar 
cd arm-performance-libraries_24.04_deb/
sudo ./arm-performance-libraries_24.04_deb.sh 
# You'll have to accept their license agreement. Type yes as answers
sudo apt install environment-modules
# Now you need to log out of your shell and log in back again
export MODULEPATH=$MODULEPATH:/opt/arm/modulefiles/
module load armpl/24.04.0_gcc
# From the same shell do
make run_cc_armpl
```
Requires [ArmPL](https://developer.arm.com/Tools%20and%20Software/Arm%20Performance%20Libraries) to be installed on system.

Also requires the environment-modules package for your OS / Distro [Environment Modules](https://modules.sourceforge.net/)

**Apple Accelerate**

This build enables BLAS acceleration via Apple Accelerate on Mac OS - Testing

```bash
make run_cc_mac_accel
```
Requires [Apple Accelerate](https://developer.apple.com/accelerate/) to be available on system.

Note: Needs testing.

**Generic CBLAS**

This build enables acceleration with any Netlib CBLAS interface compatible libraries

```bash
make run_cc_cblas
```

Requires any BLAS library with Netlib CBLAS interface such as [LAPACK](https://www.netlib.org/lapack) to be installed on system.

**CLBlast (GPU/OpenCL)**

This build enables tuned GPU acceleration via OpenCL with CLBlast

```bash
make run_cc_clblast
```

Requires [CLBlast](https://github.com/CNugteren/CLBlast) compiled with `cmake -DNETLIB=ON` to be installed on system.

Note: Currently runs much slower than CPU! Requires investigation or memory I/O is a bottle neck on the test system.

## Portable Binary Build

Have you ever wanted to inference a baby Llama 2 model with a single executable on any OS or *as OS? No? Well, now you can!

By making use of the Cosmopolitan libc toolchain to build llama2.c we get the we can get those features.

Instructions

Get and build the comopolitan libc toolchain:

Follow instructions at https://github.com/jart/cosmopolitan

Or do:

```
sudo mkdir -p /opt
sudo chmod 1777 /opt
git clone https://github.com/jart/cosmopolitan /opt/cosmo
export PATH="/opt/cosmo/bin:/opt/cosmos/bin:$PATH"
echo 'PATH="/opt/cosmo/bin:/opt/cosmos/bin:$PATH"' >>~/.profile
cosmocc --update   # pull cosmo and build/rebuild toolchain
```

Example build to generate a Actually Portable Executable (APE) with embedded model:

```bash
mkdir out
wget https://huggingface.co/karpathy/tinyllamas/resolve/main/stories15M.bin -O out/model.bin
make run_cosmocc_incbin
```

Example build to generate a APE:

```bash
make run_cosmocc
```

Run or copy to any supported system and run:

If model is embedded:

```bash
./run.com
```

Else

```bash
/run.com model.bin
```

## All 'make' targets

Do make <target> to build for a particular target.

Example:

```bash
make run_cc_openmp
```

Usage & Targets:

```
Usage:
  make <target>

Simple Builds
  run_cc                        - Standard build with basic optimizations
  runq_cc                       - Same for quantized build
  run_cc_fast                   - More Optimized build. Disregards strict standards compliance
  runq_cc_fast                  - Same for quantized build
  run_cc_gnu                    - Optimized Generic linux distro build
  runq_cc_gnu                   - Same for quantized build

Accelerated Builds
  run_cc_avx                    - ***NEW*** AVX accelerated build
  run_cc_openmp                 - OpenMP accelerated build
  runq_cc_openmp                - Same for quantized build
  run_cc_openacc                - OpenACC accelerated build
  runq_cc_openacc               - Same for quantized build
  run_cc_omp_gnu                - Generic linux distro + OpenMP build
  runq_cc_omp_gnu               - Same for quantized build
  run_cc_clblast                - CLBlast OpenCL CBLAS GPU accelerated build
  runq_cc_clblast               - Same for quantized build
  run_cc_openblas               - Openblas CBLAS accelerated build
  runq_cc_openblas              - Same for quantized build
  run_cc_cblas                  - Generic CBLAS accelerated build
  runq_cc_cblas                 - Same for quantized build
  run_cc_blis                   - BLIS accelerated build
  runq_cc_blis                  - Same for quantized build

Special Builds 

---> x86_64
  run_cc_mkl                    - ***NEW*** OpenMP + Intel MKL CBLAS build (x86_64 / intel Mac)
  runq_cc_mkl                   - Same for quantized build

---> ARM64 / aarch64
  run_cc_armpl                  - ARM PL BLAS accelerated build (ARM64 & Mac)  (WIP)
  runq_cc_armpl                 - Same for quantized build

---> Macintosh
  run_cc_mac_accel              - Mac OS OPENMP + CBLAS via Accelerate Framework build (WIP/TEST)
  runq_cc_mac_accel             - Same for quantized build

---> Windows
  run_win                       - Optimized Windows build with MinGW-w64 toolchain
  runq_win                      - Same for quantized build
  run_win_msvc                  - OpenMP accelerated Windows build with MSVC toolchain (Untested)
  runq_win_msvc                 - Same for quantized build

---> MultiOS Builds (using cosmopolitan libc + toolchain)
  run_cosmocc                   - Optimized Portable + cosmocc (runs on all OSes)
  runq_cosmocc                  - Same for quantized build

---> MultiOS Builds ---> with Embedded Models
  run_cosmocc_zipos             - Optimized Portable + cosmocc + embedded zip model build (runs on all OSes)
  runq_cosmocc_zipos            - Same for quantized build
  run_cosmocc_incbin            - Optimized Portable + cosmocc + embedded model fast build (runs on all OSes)
  runq_cosmocc_incbin           - Same for quantized build
  run_cosmocc_strlit            - Optimized Portable + cosmocc + embedded model build (runs on all OSes)
  runq_cosmocc_strlit           - Same for quantized build

---> GCC/Clang Embedded Model Builds
  run_gcc_openmp_incbin         - Gcc + OpenMP + embedded model fast build
  runq_gcc_openmp_incbin        - Same for quantized build
  run_gcc_openmp_strlit         - Gcc + OpenMP + embedded model build
  runq_gcc_openmp_strlit        - Same for quantized build
  run_clang_openmp_incbin       - Clang + OpenMP + embedded model fast build
  runq_clang_openmp_incbin      - Same for quantized build
  run_clang_openmp_strlit       - Clang + OpenMP + embedded model build
  runq_clang_openmp_strlit      - Same for quantized build

---> GCC/Clang Embedded Model Builds ---> Statically Linked
  run_gcc_static_incbin         - Optimized Static gcc + embedded model fast build
  runq_gcc_static_incbin        - Same for quantized build
  run_gcc_static_strlit         - Optimized Static gcc + embedded model build
  runq_gcc_static_strlit        - Same for quantized build
  run_clang_static_incbin       - Optimized Static clang + embedded model fast build
  runq_clang_static_incbin      - Same for quantized build
  run_clang_static_strlit       - Optimized Static clang + embedded model build
  runq_clang_static_strlit      - Same for quantized build

---> Android
  run_incbin_tmux               - Optimized build + Embedded Model for Termux on Android
  runq_incbin_tmux              - Same for quantized build

Post Compile Optimizations

Binary Optimization 
  run_bolt                      - ***NEW*** Apply llvm bolt binary optimization

Strip Symbols 
  run_strip                     - ***NEW*** Strip symbols to make binaries smaller

---> L2E Unikernel (Asteroid)
  l2e_unik_qemu                 - L2E Unikernel (Asteroid) for kvm / qemu x86_64

---> L2E Unikernel (Latest) (Asteroid)
  l2e_unik_qemu_latest          - L2E Unikernel (Latest unikraft unikernel) (Asteroid) for kvm / qemu x86_64

---> L2E Unikernel (Asteroid) ---> Boot in qemu
  boot_l2e_unik                 - Boot L2E Unikernel (Asteroid) in qemu

---> L2E OS (Humanoid)
  l2e_os                        - L2E OS, kernel module and userspace build

---> L2E OS (Humanoid) ---> Make Bootable ISO
  l2e_os_iso                    - Make Bootable L2E OS Hybrid UEFI/BIOS ISO Image

---> L2E OS (Humanoid) ---> Boot in qemu
  boot_l2e_os                   - Boot L2E OS (Humanoid) in qemu

---> L2E OS (Humanoid) ---> Boot ISO in qemu
  boot_l2e_iso                  - Boot L2E OS ISO Image in qemu

---> L2E OS (Humanoid) ---> Boot ISO with UEFI in qemu
  boot_l2e_iso_uefi             - Boot L2E OS ISO Image with UEFI in qemu

Debug Build
  run_debug                     - Debug build which can be analyzed with tools like valgrind.
  run_cc_bcdebug                - ***NEW*** Emit LLVM bitcode & transpile to C debug build
  runq_cc_bcdebug               - Same for quantized build
  run_cc_mmdebug                - ***NEW*** Matmul Debug Log build (Warning: Huge Logs)

Testing
  test                          - run all tests (inclusive python code, needs python)
  testc                         - run only tests for run.c C implementation (needs python)
  testcc                        - run the C tests, without touching pytest / python

Clean/ Purge
  tempclean                     - Find and delete all temporary files left by editors  
  clean                         - Simple cleaning 
  distclean                     - Deep cleaning (distclean sub projects)
  mintclean                     - Revert to mint condition (remove sub projects)

Misc
  get_model                     - Get stories15M model
  list                          - Display sorted list of all targets

Help
  help                          - Display this help. Make without target also displays this help.
```

All builds with embedded models need the model to be in ``out/`` directory and the model name has to be ``model.bin``

Example:

```bash
mkdir out
wget https://huggingface.co/karpathy/tinyllamas/resolve/main/stories15M.bin -O out/model.bin
```

## TODO

- [ ] Clean up README.md
- [ ] Add every project used to credits
 
## Changelog

See commits. Going forward we will diverge wildly from karpathy llama2.c

## Contributing

- All pull requests that are merged to upstream will be automatically applied here as we closely mirror upstream. 
- I merge pull requests that improves performance even if they are rejected upstream.
- Performance and usability improvement contriubutions are welcome.

# Gratitude & Credits

Thank you to to the creators of the following libraries and tools and their contributors:

- [Meta](https://llama.meta.com/) - @facebook - Creators of llama2 and llama3
- [llama2.c](https://github.com/karpathy/llama2.c) - @karpathy - The initiator and guru
- [cosmopolitan](https://github.com/jart/cosmopolitan) - @jart - Toolchain that makes write once run anyehere possible
- [OpenBlas](https://github.com/xianyi/OpenBLAS) - @xianyi - BLAS acceleration
- [blis](https://github.com/flame/blis) - @flame - BLIS BLAS acceleration
- [CLBlast](https://github.com/CNugteren/CLBlast) - @CNugteren - OpenCL BLAS acceleration
- [incbin](https://github.com/graphitemaster/incbin) - @graphitemaster - Include assets in binaries
- [strliteral](https://github.com/mortie/strliteral) - @mortie - Include assets in binaries
- [unikraft](https://github.com/unikraft) - @unikraft - Run as unikernel
- [linux](https://https://www.kernel.org/) - @torvalds - Kernel used in L2E OS
- [limine](https://github.com/limine-bootloader/limine) - @mintsuki - Bootloader for L2E OS
- [llama3.c](https://github.com/jameswdelancey/llama3.c) - @jameswdelancey - export script for llama tokenizer
- Many more 


## Other cool and notable projects
- [llama.cpp](https://github.com/ggerganov/llama.cpp)
- [llama2.c](https://github.com/karpathy/llama2.c)
- [llamafile](https://github.com/Mozilla-Ocho/llamafile)
- [llama3.c](https://github.com/jameswdelancey/llama3.c)

## License

L2E NCRL, MIT, GNU GPL, BSD and other depending on build options

# License Notice

## Original Code
Original repository contents are licensed under the MIT License and remain the copyright of their respective contributors.

## L2E Components
Modifications and additions by Vulcan Ignis are licensed under the L2E Non-Commercial Research License (L2E NCRL), permitting research and non-commercial use with attribution. Copyright © Vulcan Ignis.

## Dual License Structure
- Original components: MIT License
- L2E components: L2E Non-Commercial Research License (prohibits commercial use, requires attribution)

## Additional Components
- Tools, libraries, and dependencies used during conditional compilation: Subject to their respective licenses
- GNU/Linux kernel L2E module glue code: GNU GPL v2, Copyright © Vulcan Ignis

**Note:** All outputs including demonstrations, binaries, generated formats, and releases are governed by the L2E Non-Commercial Research License.
